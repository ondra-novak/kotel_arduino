#pragma once

#include <type_traits>
#include <stdint.h>
#include <string.h>

template<unsigned int sector_data_size,
         unsigned int directory_size,
         unsigned int page_size,
         unsigned int eeprom_size,
         typename BlockDevice>
class EEPROM {
public:



    using FileNr = uint8_t;

    static_assert(directory_size < 128, "127 entries is hard limit");

    struct HeaderType {
        uint8_t file_nr_flag = 0xFF;
        uint8_t crc8 = 0xFF;

        constexpr void set_file_nr_and_flag(uint8_t filenr, bool tombstone) {
            file_nr_flag = (filenr & 0x7F) | (tombstone?0x80:0);
        }
        constexpr uint8_t get_file_nr() const {return file_nr_flag & 0x7F;}
        constexpr bool is_tombstone() const {return (file_nr_flag & 0x80) != 0;}

        constexpr bool is_free_sector() const {return file_nr_flag == 0xFF;}
    };


    static constexpr unsigned int sector_size = sector_data_size + sizeof(HeaderType);
    static constexpr unsigned int sectors_per_page = page_size / sector_size;
    static constexpr unsigned int page_count = eeprom_size/page_size;
    static constexpr unsigned int total_sectors = sectors_per_page * page_count;
    static constexpr unsigned int file_nr_shift = 0;
    static constexpr unsigned int total_files = directory_size;

    using SectorIndex = uint16_t;
    using PageIndex = uint8_t;
    using BlockSize = decltype(std::declval<BlockDevice>().get_erase_size());
    using FlashAddr = BlockSize;

    static constexpr SectorIndex invalid_sector = ~SectorIndex{};

    static_assert(directory_size <= total_sectors - 2*sectors_per_page, "Files must be less than available sectors minus two pages (two pages are reserved)");

    struct Sector {
        HeaderType header;
        char data[sector_data_size];
        constexpr Sector() {
            for (char &x: data) x = '\xFF';
        }
        template<typename T>
        operator T() const {
            static_assert(sizeof(T) <= sizeof(data));
            return *reinterpret_cast<const T *>(data);
        }

    };

    struct FileInfo {
        SectorIndex sector = invalid_sector;
    };


    ///construct object
    /** @param dev Reference to block device
     *
     * @note don't forget to call begin()!
     *
     * */
    constexpr EEPROM(BlockDevice &dev):_flash_device(dev) {}

    ///construct object
    /** uses current instance of block device
     *
     * @note don't forget to call begin()!
     *
     * */
    constexpr EEPROM(): EEPROM(BlockDevice::getInstance()) {}


    ///Initialize object, load and build metadata
    void begin() {
        rescan();
    }

    ///rescan flash device and build file allocation table
    void rescan() {

        PageIndex free_page = page_count;
        //probe each page
        for (SectorIndex i = 0; i < total_sectors; i+=sectors_per_page) {
            FlashAddr addr = sector_2_addr(i);
            uint8_t b = 0;
            _flash_device.read(&b, addr, sizeof(b));
            if (b == 0xFF) {
                //found empty page
                free_page = sector_2_page(i);
                break;
            }

        }
        //track area of each file
        uint8_t files_area[directory_size];
        //erase directory
        for (unsigned int i = 0; i < directory_size; ++i) {
            files_area[i] = 0xFF;
            _files[i].sector = invalid_sector;
        }

        //invalidate head pointer
        SectorIndex head = total_sectors;
        //contains first sector of continuous space at the end of the EEPROM.
        SectorIndex second_head = 0;

        //we starting at area 1
        uint8_t area = 1;

        //scan whole EEPROM
        for (SectorIndex i = 0; i < total_sectors; ++i) {
            Sector s = read_sector(i);
            //it is free sector
            if (s.header.is_free_sector()) {
                //if we not found head yet
                if (head == total_sectors) {
                    //this is our head
                    head = i;
                    ++area;
                }
            } else {
                second_head = i + 1;
                //check crc
                if (s.header.crc8 == calc_crc_sector(s)) {
                    //take file number
                    FileNr fnr = s.header.get_file_nr();
                    //take tombstone flag
                    bool ts = s.header.is_tombstone();
                    //fnr range and area, if we are in the same or better area
                    if (fnr < directory_size && files_area[fnr] >= area) {
                        //update area
                        files_area[fnr] = area;
                        //update directory
                        _files[fnr].sector = ts?invalid_sector:i;
                    }
                } else {
                    //update crc error counter
                    ++_crc_errors;
                }
            }
        }

        //no head found (no free sector)
        if (head == total_sectors) {
            //run deep analysis to find unused page
            auto p = select_unused_page();
            //set head to begin of selected
            head = page_2_sector(p);
            //set this page as free (will be erased)
            free_page = p;

            //found head, but not free page
        } else if (free_page == page_count) {
            //assume that free page was not erased (power failure?)
            //set free page as page next to head sector
            free_page = sector_2_page(calc_write_stop(head));
        } else {
            //if free_page is zero, the head is zero as well.
            //if second_head is valid, then we use second head as our head
            if (free_page == 0 && second_head != total_sectors) {
                head = second_head;
            }
        }

        //ensure that free page is erased
        erase_page(free_page);
        //save  write position
        _write_pos = head;
        _free_page = free_page;
    }

    ///read file
    /**
     * @param id id of file
     * @param out_data structure into data are placed
     * @retval true success
     * @retval false not found
     */
    template<typename T>
     bool read_file(unsigned int id, T &out_data) {
        static_assert(sizeof(T) <= sector_data_size && std::is_trivially_copy_constructible_v<T>);
        if (id >= total_files) return false;
        //find file in file table
        FileInfo &f = _files[id];
        //if not exists return false
        if (f.sector == invalid_sector) return false;
        //read sector
        Sector s = read_sector(f.sector);
        if (s.header.crc8 != calc_crc_sector(s)) { //bad crc - flash is corrupted, rescan
            ++_crc_errors;
            rescan();
            return read_file(id, out_data);
        }
        //extract data
        out_data = *reinterpret_cast<const T *>(s.data);
        return true;
    }

    ///write file
    /**
     * @param id file identifier
     * @param out_data data to write. The size of data is limited to sector size - 2.
     * If sector_size is 32, you can write 30 bytes per file.
     *
     * @note unused space is left uninitalized
     *
     * @retval true success
     * @retval false invalid file id
     */
    template<typename T>
     bool write_file(unsigned int id, const T &data) {
        static_assert(sizeof(T) <= sector_data_size && std::is_trivially_copy_constructible_v<T>);
        //create new sector
        Sector s;
        s.header.set_file_nr_and_flag(id, false);
        //copy data
        memcpy(s.data, &data, sizeof(T));
        //write
        return write_file_sector(s);
    }

    ///Update file
    /** Function can be used to optimize writing into flash. However it
     * is slower to processing. The function compares stored value with
     * new value. If value is the same, no writing is performed. If value
     * can be written without erasing (only resets bits), it overwrites the
     * sector. Otherwise normal write is performed
     *
     * @param id file id
     * @param data data
     * @retval true success
     * @retval false invalid file id
     */

    template<typename T>
    bool update_file(unsigned int id, const T &data) {
        static_assert(sizeof(T) <= sector_data_size && std::is_trivially_copy_constructible_v<T>);
        //id out of range
        if (id >= total_files) return false;

        auto cur_sector = _files[id].sector;
        //file must exists
        if (cur_sector != invalid_sector) {
            //read current sector
            Sector cur = read_sector(cur_sector);
            //must have valid crc
            if (cur.header.crc8 == calc_crc_sector(cur)) {
                //compare bytes
                const char *new_bytes = reinterpret_cast<const char *>(&data);
                if (std::equal(new_bytes, new_bytes+sizeof(T), cur.data)) {
                    //bytes equal, no write is needed
                    return true;
                }
            }
        }
        //write new revision
        return write_file(id, data);
    }

    ///Lists all active revisions of a file
    /**
     * This function can be used to retrieve old revision if they were not deleted.
     * Function must scan whole eeprom to find all revisions (slow).
     *
     * Revisions are ordered from oldest to newest
     *
     * @param id file id
     * @param fn function receives content - you can read it
     * as const T & or as Sector
     *
     *
     *
     */
    template<typename Fn>
    void list_revisions(unsigned int id, Fn &&fn) {
        //check type of lambda function
        static_assert(std::is_invocable_v<Fn, const Sector &>, "void(const Sector &)");
        //start oldest area
        auto idx = calc_write_stop(_write_pos);
        //until end
        while (idx < total_sectors) {
            Sector s = read_sector(idx);
            //read sector, check type and check file nr
            if (!s.header.is_free_sector() && !s.header.is_tombstone()
                    && s.header.crc8 == calc_crc_sector(s) && s.header.get_file_nr()== id) {
                //then report
               fn(s);
            }
            ++idx;
        }
        //calculate end of second area
        auto idx_end = _write_pos;
        //start by sector 0
        idx = 0;
        while (idx < idx_end) {
            Sector s = read_sector(idx);
            //read sector, check type and check file nr
            if (!s.header.is_free_sector() && !s.header.is_tombstone()
                    && s.header.crc8 == calc_crc_sector(s) && s.header.get_file_nr()== id) {
                //then report
               fn(s);
            }
            ++idx;
        }
    }

    ///return true after begin() if eeprom is in error state
    constexpr bool is_error() const {
        return _error;
    }

    ///retrieve crc error counter
    /**
     * @return everytime a crc error encountered, counter is increased.
     * If returned values is not zero, it shows that EEPROM is in
     * bad condition and should be replaced
     */
    constexpr unsigned int get_crc_error_counter() const {
        return _crc_errors;
    }

    ///Erase file - so file appears erased
    /**
     * @param id file number
     * @retval true file erased
     * @retval false file not erased
     *
     * @note erased file may be not erased now, but eventually it is erased
     * during wear leveling
     */
    bool erase_file(unsigned int id) {
        //id out of range
        if (id >= total_files) return false;
        //file isn't deleted
        if (_files[id].sector != invalid_sector) {
            //write tombstone
            Sector s;
            s.header.set_file_nr_and_flag(id, true);
            write_file_sector(s);
        }
        return true;
    }

    ///returns count active files
    unsigned int file_count() const {
        unsigned int cnt = 0;
        for (const auto &x: _files) {
            if (x.sector != invalid_sector) ++cnt;
        }
        return cnt;
    }

    ///returns size occupied by files (including headers)
    unsigned int size() const {
        return file_count() * sector_size;
    }

    ///returns size of data
    unsigned int data_size() const {
        return file_count() * sector_data_size;
    }

    ///returns whether it is empty
    bool empty() const {
        return file_count() == 0;
    }

    void erase_all() {
        for (unsigned int i = 0; i < page_count; ++i) {
            erase_page(i);
        }
        for (unsigned int i = 0; i < directory_size; ++i) {
            _files[i].sector = invalid_sector;
        }
        _write_pos = 0;
        _free_page = 0;
    }

protected:

    BlockDevice &_flash_device;
    FileInfo _files[total_files] = {};
    SectorIndex _write_pos;
    PageIndex _free_page;

    bool _error = false;
    unsigned int _crc_errors = 0;

    ///Reads sector
     Sector read_sector(SectorIndex idx) {
        Sector s;
        FlashAddr addr = sector_2_addr(idx);
        _flash_device.read(&s, addr, sizeof(s));
        return s;
    }



    ///Finds unused page
    /**
     * This function is called when rescan() fails to find empty page. The
     * empty page is page lying right after page having unused sector, or
     * page when unused sector is found at the beginning of the page. If
     * no unused sector was found, this is strange situation which can be solved
     * by scanning all files and finding page which doesn't contain any active file
     *
     * @return index of unused page or -1 if this process failed
     */
     constexpr PageIndex select_unused_page() const {
         //the first strategy - mark every page which contains file/active sector
        bool page_map[page_count] = {};
        //process all files
        for (unsigned int i = 0 ; i < total_files; i++) {
            const FileInfo &f = _files[i];
            if (f.sector != invalid_sector) {
                PageIndex idx = f.sector / sectors_per_page;
                //mark page
                page_map[idx] = true;
            }
        }
        //find page which has no mark
        for (PageIndex i = 0; i < page_count; ++i) {
            //this is our page
            if (!page_map[i]) return i;
        }
        //TODO: second strategy, find page which is backed by other pages

        //fail save - select last page as free (something will be erased)
        return page_count -1;
    }

    ///Erase specific page
    /**
     * The page is filled by FFs
     * @param index index of page
     */
     void erase_page(PageIndex index) {
        FlashAddr addr = static_cast<FlashAddr>(index) * page_size;
        _flash_device.erase(addr, page_size);
    }

    ///Append sector
    /**
     * Note function doesn't check, whether page is erased! This is
     * raw copy
     *
     * @param s sector to write
     * @return index where sector has been written
     */
     SectorIndex append_sector(const Sector &s) {
         SectorIndex idx = _write_pos;
        ++_write_pos;
        write_sector(idx, s);
        return idx;
    }

     ///Write sector at addres
     /**
      * Note function doesn't check, whether page is erased! This is
      * raw copy
      *
      * @param idx sector addres
      * @param s sector to write
      */
      void write_sector(SectorIndex idx, const Sector &s) {
         FlashAddr addr = sector_2_addr(idx);
         _flash_device.program(&s, addr, sizeof(s));
     }


     ///write sector with file
     /**
      * @param s sector with data, must have initialized header for file_nr_flag;
      * @retval true success
      * @retval false failure
      *
      * @note performs defragmentation if needed
      */
     bool write_file_sector(Sector &s) {

        //retrieve id
        FileNr id = s.header.get_file_nr();
        //id out of range - error
        if (id >= total_files) return false;
        //retrieve file directory record
        FileInfo &f = _files[id];
        //upda
        s.header.crc8 = calc_crc_sector(s);
        //check whether current write position is at stop index
        //if does, defragmentation is needed
        if (_write_pos == calc_write_stop(_write_pos)) {
            //contains next page to erase
            auto nx_page = _free_page;
            //contains count of files/active sectors on that page
            unsigned int files_on_page = 0;
            //contains index of first sector
            SectorIndex nx_sect_beg = 0;
            //contains index of last sector + 1
            SectorIndex nx_sect_end = 0;
            do {
                files_on_page = 0;
                //move to next page
                nx_page = (nx_page + 1) % page_count;
                //check whether we tested all pages - then error
                if (nx_page == _free_page) return false;
                //calculate beg and end
                nx_sect_beg = page_2_sector(nx_page);
                nx_sect_end = nx_sect_beg + sectors_per_page;
                //enumerate all files and count how many are
                for (FileNr x = 0; x < directory_size; ++x) if (x != id) {
                    //take sector index
                    SectorIndex xs = _files[x].sector;
                    //if sector index is in range, increase counter
                    files_on_page += (xs >= nx_sect_beg && xs < nx_sect_end)?1:0;
                }
                //if we filled whole page, we cannot use this page, find another
            } while (files_on_page >= sectors_per_page);
            //so we have selected new page
            //we can update _write_pos
            _write_pos = page_2_sector(_free_page);
            //write the sector to the _free_page
            f.sector = append_sector(s);

            if (files_on_page > 0) {
                //copy all other files from next free page to current _free page
                for (FileNr x = 0; x < directory_size; ++x) if (x != id) {
                    SectorIndex &xs = _files[x].sector;
                    if (xs >= nx_sect_beg && xs < nx_sect_end) {
                        //read sector
                        Sector ss = read_sector(xs);
                        if (ss.header.crc8 != calc_crc_sector(ss)) {
                            ++_crc_errors;
                        } else {
                            //copy sector
                            xs = append_sector(ss);
                        }
                    }
                }
            }
            //next free page is no longer needed we copied everything relevant
            //this is now out _free_page
            _free_page = nx_page;
            //erase it
            erase_page(_free_page);
        } else {
            //no defragmentation, just write sector
            f.sector = append_sector(s);
        }

        //if tombstone writen, erase file, otherwise store its position
        if (s.header.is_tombstone()) f.sector = invalid_sector;
        //this is success
        return true;
    }


     static constexpr FlashAddr sector_2_addr(SectorIndex idx) {
         if constexpr(sector_size * sectors_per_page == page_size) {
             return static_cast<FlashAddr>(idx) * sector_size;
         } else {
             PageIndex page = idx / sectors_per_page;
             SectorIndex sect_in_page = idx % sectors_per_page;
             return static_cast<FlashAddr>(page) * page_size +
                     static_cast<FlashAddr>(sect_in_page) * sector_size;
         }
     }

     static constexpr uint16_t crc16_update(uint16_t crc, uint8_t a) {
     int i = 0;
     crc ^= a;
     for (i = 0; i < 8; ++i)
     {
         if (crc & 1)
         crc = (crc >> 1) ^ 0xA001;
         else
         crc = (crc >> 1);
     }
     return crc;
     }

     static SectorIndex page_2_sector(PageIndex idx) {
         return static_cast<SectorIndex>(idx) * sectors_per_page;
     }

     static PageIndex sector_2_page(SectorIndex idx) {
         return static_cast<PageIndex>(idx / sectors_per_page);
     }

     static constexpr uint8_t crc8table[2][16] = {
         {0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
         0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41},
         {0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
         0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74}
     };

     static constexpr uint8_t crc8_update(uint8_t byte, uint8_t crc) {
             crc = byte ^ crc;
             auto h1 = crc & 0xF;
             auto h2 = (crc >> 4);
             crc = crc8table[0][h1] ^ crc8table[1][h2];
             return crc;
     }

     static constexpr uint8_t calc_crc_sector(const Sector &sec) {
         uint8_t res = crc8_update(sec.header.file_nr_flag, 0);
         for (uint8_t x: sec.data) {
             res = crc8_update(x, res);
         }
         return res;
     }

     static constexpr SectorIndex calc_write_stop(SectorIndex idx) {
         auto p =  (idx + sectors_per_page -1 )/ sectors_per_page;
         return p * sectors_per_page;
     }


};

