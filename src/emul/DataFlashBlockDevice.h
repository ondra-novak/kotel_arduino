#include <cstddef>
#include <iterator>

#define FLASH_BLOCK_SIZE 1024
#define FLASH_TOTAL_SIZE 8192


class DataFlashBlockDevice {
public:

    DataFlashBlockDevice() {
        std::fill(std::begin(_data), std::end(_data), '\xFF');
    }

    static constexpr std::size_t get_erase_size() {return 1024;}

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

    static DataFlashBlockDevice &getInstance() {
        static DataFlashBlockDevice inst;
        return inst;
    }

    char _data[8192];
};

