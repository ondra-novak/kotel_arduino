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

    enum class SectorType: uint8_t {
        free = 0xFF,
        file = 0xFE,
        tombstone = 0xFC
    };

    using FileNr = uint8_t;

    static_assert(directory_size <= 256, "256 is hard limit");

    struct HeaderType {
        SectorType type = SectorType::free;
        FileNr file_nr = 0xFF;
        uint16_t crc = 0xFFFF;
    };

    static constexpr bool is_tombstone(uint16_t x) {
        return x & 1;
    }

    static constexpr unsigned int get_file_no(uint16_t x) {
        return x >> 1;
    }

    static constexpr uint16_t get_file_no_and_flags(int fileno, bool tombstone) {
        return static_cast<uint16_t>((fileno << 1) | (tombstone?1:0));
    }

    static constexpr unsigned int sector_size = sector_data_size + sizeof(HeaderType);
    static constexpr unsigned int sectors_per_page = page_size / sector_size;
    static constexpr unsigned int page_count = eeprom_size/page_size;
    static constexpr unsigned int total_sectors = sectors_per_page * page_count;
    static constexpr uint16_t free_sector = 0xFFFF;
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

        /* flash area is split into two areas
         * the 1st area is after empty page
         * the 2nd area is before empty page
         *
         * If the empty page is the first, there is no the 2nd area
         * if the empty page is the last, there is no the 1st area
         *
         * Revisions are go from oldest to newest in both areas. However
         * the 2nd area is newer then 1st area.
         *
         * When scanning sectors, sector of files before empty page has
         * higher priority. Any file found there is recorded even if
         * already exists (because new sector is second area is newer revision);
         *
         * When scanning sectors after empty page, already existing files
         * are not overwritten because this area contains older revisions..
         * However files found new in this area are updated during
         * processing this area
         *
         *
         */
        uint8_t file_tracker[directory_size] = {};
        //current area (we starting by 2, then by 1
        /* the numbering is because 0 is "not found" which is "like 3rd area" */
        uint8_t cur_area = 2;
        //scan error can cause, when no empty page is found or created
        _error = false;

        //
        SectorIndex last_used = total_sectors-1;
        for (unsigned int i = 0; i < total_files; ++i) {
            _files[i].sector = invalid_sector;
        }
        bool free_found = false;
        for (SectorIndex i = 0; i < total_sectors; ++i) {
            Sector sec = read_sector(i);
            if (sec.header.type == SectorType::free) {
                if (!free_found) {
                    _first_free_sector = i;
                    cur_area = 1;
                    free_found = true;
                }
            } else {
                auto crc = calc_crc_sector(sec);
                if (crc == sec.header.crc) {
                    bool tombstone = sec.header.type == SectorType::tombstone;
                    auto fno = sec.header.file_nr;
                    if (file_tracker[fno] <= cur_area) {
                        file_tracker[fno] = cur_area;
                        _files[fno].sector = tombstone?invalid_sector:i;
                    }
                    last_used = i;
                } else {
                    _crc_errors++;
                }
            }
        }
        if (free_found) {
            //if first free sector is sector 0 and there is at least some sectors at the end
            if (_first_free_sector == 0 && (last_used + 1U < total_sectors)) {
                //we can continue write after last sector
                _first_free_sector = last_used + 1U;
                //and free page is first
                _first_free_page = 0;
            } else {
                //otherwise free page is next page relative to first free sector
                //unless first free sector is at the beginning of the
                //page, then free page is same page where the first free sector is
                _first_free_page = ((_first_free_sector + sectors_per_page -1) / sectors_per_page) % page_count;
            }


        } else {
            //when no free sector found,
            //then page could not be erased because power failure
            //we can find some empty page by reading metadata
            auto p = find_unused_page();
            //if free page was not found, this is error and we can't continue
            if (p == ~PageIndex{}) {
                _error = true;
                return ;
            } else {
                //mark free page
                _first_free_page = p;
                //mark free sector
                _first_free_sector = _first_free_page * sectors_per_page;
            }
        }
        //always erase selected free page (ensure, that page is really erased)
        erase_page(_first_free_page);
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
        if (s.header.crc != calc_crc_sector(s)) { //bad crc - flash is corrupted, rescan
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
        s.header.file_nr = id;
        s.header.type = SectorType::file;
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
            if (cur.header.crc == calc_crc_sector(cur)) {
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
        PageIndex start_page = _first_free_page + 1;
        auto idx = start_page * sectors_per_page;
        //until end
        while (idx < total_sectors) {
            Sector s = read_sector(idx);
            //read sector, check type and check file nr
            if (s.header.type == SectorType::file && s.header.file_nr == id
                    //and crc
                    && s.header.crc == calc_crc_sector(s)) {
                //then report
               fn(s);
            }
            ++idx;
        }
        //calculate end of second area
        auto idx_end = _first_free_page * sectors_per_page;
        //start by sector 0
        idx = 0;
        while (idx < idx_end) {
            Sector s = read_sector(idx);
            //read sector, check type and check file nr
            if (s.header.type == SectorType::file && s.header.file_nr == id
                    //and crc
                    && s.header.crc == calc_crc_sector(s)) {
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
            s.header.file_nr = id;
            s.header.type = SectorType::tombstone;
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

protected:

    BlockDevice &_flash_device;
    FileInfo _files[total_files] = {};
    SectorIndex _first_free_sector = 0;
    PageIndex _first_free_page = 0;
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
     constexpr PageIndex find_unused_page() const {
        bool page_map[page_count] = {};
        for (unsigned int i = 0 ; i < total_files; i++) {
            const FileInfo &f = _files[i];
            if (f.sector != invalid_sector) {
                PageIndex idx = f.sector / sectors_per_page;
                page_map[idx] = true;
            }
        }
        for (PageIndex i = 0; i < page_count; ++i) {
            if (!page_map[i]) return i;
        }
        return ~PageIndex{};
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
        SectorIndex idx = _first_free_sector;
        _first_free_sector = (_first_free_sector+1) % total_sectors;
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

    ///Copies all files stored in specified page, except specified file
    /**
     * This function is used to free a page. Because it is used while writing
     * new revision of a file, the specified file is not copied
     *
     * The function can copy complete page, if all files are in the page but not
     * excluded file. The function will never copy more then sectors_per_page files,
     * so result can be copy whole page to new page
     *
     * @param from_page page number (it is expected to be erased)
     * @param exclude_file_nr file number to not copy
     * @return
     */
     bool copy_page_except_file(PageIndex from_page, unsigned int exclude_file_nr) {
        SectorIndex page_sector = from_page * sectors_per_page;
        bool found = false;
        for (unsigned int i = 0; i < total_files; ++i) {
            if (i != exclude_file_nr) {
                FileInfo &f = _files[i];
                if (f.sector >= page_sector && f.sector <= page_sector+sectors_per_page) {
                    Sector s = read_sector(f.sector);
                    SectorIndex new_s = append_sector(s);
                    f.sector = new_s;
                }
            } else {
                found = true;
            }
        }
        return found;
    }

    ///retrieve whether it is required to create new page
    /**
     * @retval true before write perform steps to create new page
     * @retval false not needed
     */
     constexpr bool need_create_page() const {
        return _first_free_sector / sectors_per_page == _first_free_page;
    }

     bool write_file_sector(Sector &s) {

        FileNr id = s.header.file_nr;
        if (id >= total_files) return false;
        FileInfo &f = _files[id];
        //check whether we need to create new page (next sector is on empty buffer page)
        bool need_erase = need_create_page();
        bool erase_at_end = need_erase;
        while (need_erase) {
            //advance first free page
            _first_free_page = (_first_free_page + 1) % page_count;
            //copy everything active in new free page before erase
            copy_page_except_file(_first_free_page, id);
            //it can happen, that result is full page
            need_erase = need_create_page();
            if (need_erase) {
                //so if page is full
                //first free page is empty
                //then we can erase it
                erase_page(_first_free_page);
                //and repeat process
            }
        }
        s.header.crc = calc_crc_sector(s);
        //write sector and retrieve its index
        f.sector = append_sector(s);
        //if sector type is tombstone, this means that file is deleted
        if (s.header.type == SectorType::tombstone) f.sector = invalid_sector;
        //if we create free page, erase it now
        if (erase_at_end) erase_page(_first_free_page);
        //success
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

     static constexpr uint16_t calc_crc_sector(const Sector &sec) {
         uint16_t res = 0xFFFF;
         for (uint8_t x: sec.data) {
             res = crc16_update(res, x);
         }
         return res;
     }

};

