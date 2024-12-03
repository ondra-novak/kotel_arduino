#include "check.h"
#include "constexpr_check.h"
#include "../generic_eeprom.h"




class EmulBlockDevice {
public:

    EmulBlockDevice() {
        std::fill(std::begin(_data), std::end(_data), '\xFF');
    }

    static constexpr std::size_t get_erase_size() {return 1024;}

    int program(const void *buffer, std::size_t addr, std::size_t size) {
        CHECK_LESS_EQUAL(addr+size, sizeof(_data));
        std::copy(reinterpret_cast<const char *>(buffer),
                reinterpret_cast<const char *>(buffer)+size,
                reinterpret_cast<char *>(_data)+addr);
        return 0;
    }
    int read(void *buffer, std::size_t addr, std::size_t size) {
        CHECK_LESS_EQUAL(addr+size, sizeof(_data));
        std::copy(_data+addr, _data+addr+size, reinterpret_cast<char *>(buffer));
        return 0;
    }

    int erase(std::size_t addr, std::size_t size) {
        CHECK_EQUAL(addr%1024,0);
        CHECK_EQUAL(size%1024,0);
        CHECK_GREATER(size,0);
        CHECK_LESS_EQUAL(addr+size, sizeof(_data));
        std::fill(reinterpret_cast<char *>(_data+addr),
                reinterpret_cast<char *>(_data+addr)+size,
                '\xFF');

        return 0;
    }

    char _data[8192];
};

template<unsigned int sector_size>
using EEPromDef = EEPROM<sector_size,15,
        EmulBlockDevice::get_erase_size(),
        EmulBlockDevice::get_erase_size()*8,
        EmulBlockDevice>;

template<unsigned int sector_size>
class TestableEEProm: public EEPromDef<sector_size> {
public:

    using EEPromDef<sector_size>::EEPromDef;


};

template<unsigned int sector_size>
void test_3_files() {
    EmulBlockDevice flash;
    constexpr uint64_t canary = 0xABABABABABABUL;
    {
        TestableEEProm<sector_size> eeprom(flash);
        eeprom.begin();
        eeprom.write_file(0,canary);
        for (int i = 0; i < 10000; ++i) {
            eeprom.write_file(1, i);
            if (!(i & 1)) eeprom.write_file(2, i);
        }
        int x;
        bool b = eeprom.read_file(1, x);
        CHECK(b);
        CHECK_EQUAL(x,9999);
        b = eeprom.read_file(2, x);
        CHECK(b);
        CHECK_EQUAL(x,9998);
        uint64_t c;
        b = eeprom.read_file(0, c);
        CHECK(b);
        CHECK_EQUAL(c,canary);
    }
    {
        TestableEEProm<sector_size> eeprom(flash);
        eeprom.begin();
        int x;
        bool b = eeprom.read_file(1, x);
        CHECK(b);
        CHECK_EQUAL(x,9999);
        b = eeprom.read_file(2, x);
        CHECK(b);
        CHECK_EQUAL(x,9998);
        uint64_t c;
        b = eeprom.read_file(0, c);
        CHECK(b);
        CHECK_EQUAL(c,canary);

        if constexpr(sector_size == 30) {
            eeprom.list_revisions(2, [ctx = 9854](int x) mutable {
                CHECK_EQUAL(ctx, x);
                ctx = ctx + 2;
            });
        } else if constexpr(sector_size == 22) {
            eeprom.list_revisions(2, [ctx = 9822](int x) mutable {
                CHECK_EQUAL(ctx, x);
                ctx = ctx + 2;
            });
        }
        eeprom.erase_file(0);
        CHECK(!eeprom.read_file(0,c));
    }
    {
        uint64_t c;
        TestableEEProm<sector_size> eeprom(flash);
        eeprom.begin();
        CHECK(!eeprom.read_file(0,c));
    }
}




int main() {
    test_3_files<32>();
    test_3_files<30>();
    test_3_files<22>();




    return 0;   //constexpr test only
}
