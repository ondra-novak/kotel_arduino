

#include <type_traits>
#include "DataFlashBlockDevice.h"
#include <stdint.h>
#include <string.h>

template<unsigned int sector_size,
         unsigned int file_nr_bits,
         unsigned int page_size,
         unsigned int eeprom_size,
         typename BlockDevice = DataFlashBlockDevice>
class EEProm {
public:
    using HeaderType = uint16_t;
    static constexpr unsigned int header_size_bits = sizeof(HeaderType) * 8;

    static constexpr unsigned int sectors_per_page = page_size / sector_size;
    static constexpr unsigned int page_count = eeprom_size/page_size;
    static constexpr unsigned int total_sectors = sectors_per_page * page_count;
    static constexpr unsigned int rev_bits = header_size_bits - file_nr_bits;
    static constexpr HeaderType free_sector = ~static_cast<HeaderType>(0);
    static constexpr HeaderType file_nr_mask = free_sector >> (rev_bits);
    static constexpr HeaderType rev_mask = free_sector >> file_nr_bits;
    static constexpr unsigned int file_nr_shift = 0;
    static constexpr unsigned int rev_shift = file_nr_bits;
    static constexpr unsigned int total_files = file_nr_mask;
    static constexpr unsigned int data_size = sector_size - sizeof(HeaderType);

    using SectorIndex = std::conditional_t<(total_sectors >= 256),uint16_t,uint8_t>;
    using PageIndex = std::conditional_t<(page_count >= 256),uint16_t,uint8_t>;
    using RevisionNr = std::conditional_t<(rev_bits > 8),uint16_t,uint8_t>;
    using FileNr = std::conditional_t<(file_nr_bits > 8),uint16_t,uint8_t>;
    using BlockSize = decltype(std::declval<BlockDevice>().get_erase_size());
    using FlashAddr = BlockSize;

    static constexpr SectorIndex invalid_sector = ~SectorIndex{};

    static_assert(rev_mask >= 2*total_sectors, "Too many files or small sectors");

    struct Sector {
        HeaderType header = free_sector;
        char data[data_size];
        Sector() {
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
        RevisionNr revision = {};
    };

    struct SectorInfo {
        FileNr file_nr;
        RevisionNr revision;
    };


    ///construct object
    /** @param dev Reference to block device */
    constexpr EEProm(BlockDevice &dev):_flash_device(dev) {
        rescan();
    }

    ///construct object
    /** uses current instance of block device */
    constexpr EEProm(): EEProm(BlockDevice::getInstance()) {}

    ///rescan flash device and build file allocation table
    void rescan() {
        _error = false;
        SectorIndex last_used = total_sectors-1;
        for (FileNr i = 0; i < total_files; ++i) {
            _files[i].sector = invalid_sector;
        }
        bool free_found = false;
        for (SectorIndex i = 0; i < total_sectors; ++i) {
            Sector sec = read_sector(i);
            if (sec.header == free_sector) {
                if (!free_found) {
                    _first_free_sector = i;
                    free_found = true;
                }
            } else {
                SectorInfo nfo = parse_header(sec.header);
                FileInfo &f = _files[nfo.file_nr];
                if (f.sector == invalid_sector || is_newer(nfo.revision, f.revision)) {
                    f.sector = i;
                    f.revision = nfo.revision;
                }
                last_used = i;
            }
        }
        if (free_found) {
            if (_first_free_sector == 0 && (last_used + 1U < total_sectors)) {
                _first_free_sector = last_used + 1;
                _first_free_page = 0;
            } else {
                _first_free_page = ((_first_free_sector + sectors_per_page -1) / sectors_per_page) % page_count;
            }


        } else {
            auto p = find_unused_page();
            if (p == ~PageIndex{}) {
                _error = true;
                return ;
            } else {
                _first_free_page = find_unused_page();
                _first_free_sector = _first_free_page * sectors_per_page;
            }
        }
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
     bool read_file(FileNr id, T &out_data) {
        static_assert(sizeof(T) <= data_size && std::is_trivially_copy_constructible_v<T>);
        if (id >= total_files) return false;
        //find file in file table
        FileInfo &f = _files[id];
        //if not exists return false
        if (f.sector == invalid_sector) return false;
        //read sector
        Sector s = read_sector(f.sector);
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
     bool write_file(FileNr id, const T &data) {
        static_assert(sizeof(T) <= data_size && std::is_trivially_copy_constructible_v<T>);
        //create new sector
        Sector s;
        //copy data
        memcpy(s.data, &data, sizeof(T));

        return write_file_sector(id, s);
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
    bool update_file(FileNr id, const T &data) {
        static_assert(sizeof(T) <= data_size && std::is_trivially_copy_constructible_v<T>);

        if (id >= total_files) return false;
        auto cur_sector = _files[id].sector;
        if (cur_sector == invalid_sector) return write_file(id, data);
        const char *new_bytes = reinterpret_cast<const char *>(data);
        Sector cur = read_sector(cur_sector);
        bool need_write = false;
        for (unsigned int i = 0; i < sizeof(T); ++i) {
            need_write = need_write || (cur.data[i] != new_bytes[i]);
            char r = cur.data[i] & new_bytes[i];
            if (r != new_bytes[i]) {
                memcpy(cur.data, new_bytes, sizeof(T));
                return write_file_sector(id, cur);
            }
            cur.data[i] = r;
        }
        if (need_write) {
            //so we successfully merged without need to erase
            write_sector(cur_sector, cur);
        }
        return true;
    }

    ///Lists all active revisions of a file
    /**
     * This function can be used to retrieve old revision if they were not deleted.
     * Function must scan whole eeprom to find all revisions (slow)
     *
     * @param id file id
     * @param fn function receives revision id and content - you can read it
     * as const T & or as Sector
     *
     * @note result is not ordered, you need to order it by revision id. You can use
     * is_newer function for ordering
     */
    template<typename Fn>
    void list_revisions(FileNr id, Fn &&fn) {
        static_assert(std::is_invocable_v<Fn, RevisionNr, const Sector &>);
        for (SectorIndex idx = 0; idx < total_sectors; ++idx) {
            Sector s = read_sector(idx);
            SectorInfo snfo = parse_header(s.header);
            if (snfo.file_nr == id) {
                fn(snfo.revision, s);
            }
        }
    }

    constexpr bool is_error() const {
        return _error;
    }

    ///Determines whether file is newer depend on revision
    /**
     * @param n new file revision
     * @param o old file revision
     * @retval true n is newer
     * @retval false n is not newer
     */
     constexpr bool is_newer(RevisionNr n, RevisionNr o) {
        RevisionNr diff = n - o;
        return diff < (rev_mask >> 1);
    }
protected:

    BlockDevice &_flash_device;
    FileInfo _files[total_files] = {};
    SectorIndex _first_free_sector = 0;
    PageIndex _first_free_page = 0;
    bool _error = false;

    ///Reads sector
     Sector read_sector(SectorIndex idx) {
        Sector s;
        FlashAddr addr = sector_2_addr(idx);
        _flash_device.read(&s, addr, sizeof(s));
        return s;
    }

    ///Parses sector header
     constexpr SectorInfo parse_header(HeaderType h) {
        SectorInfo ret;
        ret.file_nr = static_cast<FileNr>((h >> file_nr_shift) & file_nr_mask);
        ret.revision = static_cast<RevisionNr>((h >> rev_shift) & rev_mask);
        return ret;
    }

    ///Composes sector header
     constexpr HeaderType compose_header(const SectorInfo &nfo) {
        return (static_cast<HeaderType>(nfo.file_nr & file_nr_mask) << file_nr_shift)
            | (static_cast<HeaderType>(nfo.revision & rev_mask) << rev_shift);
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
        for (FileNr i = 0 ; i < total_files; i++) {
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
     bool copy_page_except_file(PageIndex from_page, FileNr exclude_file_nr) {
        SectorIndex page_sector = from_page * sectors_per_page;
        bool found = false;
        for (FileNr i = 0; i < total_files; ++i) {
            if (i != exclude_file_nr) {
                FileInfo &f = _files[i];
                if (f.sector >= page_sector && f.sector <= page_sector+sectors_per_page) {
                    Sector s = read_sector(f.sector);
                    SectorInfo sinfo = parse_header(s.header);
                    ++sinfo.revision;
                    s.header = compose_header(sinfo);
                    SectorIndex new_s = append_sector(s);
                    f.revision = sinfo.revision;
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

     bool write_file_sector(FileNr id, Sector &s) {

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
        //increase revision
        ++f.revision;
        //create header
        s.header = compose_header({id,f.revision});
        //write sector and retrieve its index
        f.sector = append_sector(s);
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


};
