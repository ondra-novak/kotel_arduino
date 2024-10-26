#pragma once
#include "DataFlashBlockDevice.h"
#include "generic_eeprom.h"


///Construct EEPROM handler
/**
 * @tparam sector_data_size size of sector (size of stored data). The sector can be
 * little bigger. There are 2 bytes extra for each sector. So value 32 creates 34
 * sector size, which results to 30 sector per page and left 4 unused bytes
 * @tparam
 */
template<unsigned int sector_data_size,
         unsigned int directory_size,
         unsigned int page_size = FLASH_BLOCK_SIZE,
         unsigned int eeprom_size = FLASH_TOTAL_SIZE,
         typename BlockDevice = DataFlashBlockDevice>
class EEPROM;


///calculates raw sector size to sector_data_size
constexpr unsigned int EEPROM_from_raw_sector_size(unsigned int raw_sector_size) {
    return raw_sector_size - 2;
}
