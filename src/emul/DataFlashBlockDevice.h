#include <cstddef>
#include <iterator>
#include <fstream>

#define FLASH_BLOCK_SIZE 1024
#define FLASH_TOTAL_SIZE 8192


class MemDataFlashBlockDevice {
public:

    MemDataFlashBlockDevice() {
        std::fill(std::begin(_data), std::end(_data), '\xFF');
    }

    static constexpr std::size_t get_erase_size() {return FLASH_BLOCK_SIZE;}

    int program(const void *buffer, std::size_t addr, std::size_t size) {
        std::copy(reinterpret_cast<const char *>(buffer),
                reinterpret_cast<const char *>(buffer)+size,
                reinterpret_cast<char *>(_data)+addr);
        return 0;
    }
    int read(void *buffer, std::size_t addr, std::size_t size) {
        std::copy(_data+addr, _data+addr+size, reinterpret_cast<char *>(buffer));
        return 0;
    }

    int erase(std::size_t addr, std::size_t size) {
        std::fill(reinterpret_cast<char *>(_data+addr),
                reinterpret_cast<char *>(_data+addr)+size,
                '\xFF');

        return 0;
    }

    static MemDataFlashBlockDevice &getInstance() {
        static MemDataFlashBlockDevice inst;
        return inst;
    }

    char _data[FLASH_TOTAL_SIZE];
};

class DiskBackedFlashBlockDevice {
public:
    DiskBackedFlashBlockDevice(std::string fname) {
        f.open(fname, std::ios::in|std::ios::out|std::ios::binary);
        if (!f) {
            f.clear();
            f.open(fname, std::ios::in|std::ios::out|std::ios::trunc|std::ios::binary);
            if (!f) throw std::runtime_error("Failed to open flash");
            for (int i = 0; i < FLASH_TOTAL_SIZE; ++i) {
                f.put('\xFF');
            }
        }
    }

    int program(const void *buffer, std::size_t addr, std::size_t size) {
        f.seekp(addr);
        f.write(reinterpret_cast<const char *>(buffer),size);
        return 0;
    }
    int read(void *buffer, std::size_t addr, std::size_t size) {
        f.seekg(addr);
        f.read(reinterpret_cast<char *>(buffer), size);
        return 0;
    }

    int erase(std::size_t addr, std::size_t size) {
        f.seekp(addr);
        for (std::size_t i = 0; i < size; ++i) {
            f.put('\xFF');
        }
        return 0;
    }

    static constexpr std::size_t get_erase_size() {return FLASH_BLOCK_SIZE;}


    static DiskBackedFlashBlockDevice &getInstance() {
        static DiskBackedFlashBlockDevice inst("/tmp/kotel_flash");
        return inst;
    }





protected:
    std::fstream f;
};

using DataFlashBlockDevice = MemDataFlashBlockDevice;

