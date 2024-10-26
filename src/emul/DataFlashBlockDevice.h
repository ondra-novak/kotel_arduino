#include <cstddef>

using bd_addr_t = std::size_t;
using bd_size_t = std::size_t;

class DataFlashBlockDevice  {
private:
    static constexpr std::size_t flash_block_size = 0x400;
    static constexpr std::size_t flash_size = 0x2000;

public:
   static DataFlashBlockDevice& getInstance();

   DataFlashBlockDevice(DataFlashBlockDevice const&) = delete;
   void operator=(DataFlashBlockDevice const&) = delete;
   ~DataFlashBlockDevice();
   int init();
   int deinit();
   int program(const void *buffer, bd_addr_t addr, bd_size_t size);
   int read(void *buffer, bd_addr_t addr, bd_size_t size);
   int erase(bd_addr_t addr, bd_size_t size);
   bd_size_t get_program_size() const  {return flash_block_size;}
   bd_size_t get_erase_size() const  {return flash_block_size;}
   bd_size_t get_read_size() const  {return flash_block_size;}
   bd_size_t size() const  {return flash_size;};

};

