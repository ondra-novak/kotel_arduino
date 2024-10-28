#pragma once
#include "nonv_storage_def.h"
#include <r4eeprom.h>

namespace kotel {
class Storage {
public:


    Config config;
    Tray tray;
    Utilization utlz;
    Counters1 cntr1;
    Counters2 cntr2;
    TempSensor temp;

    void update(const Config &x) {
        _eeprom.update_file(file_config, x);
    }
    void update(const Tray &x) {
        _eeprom.update_file(file_tray, x);
    }
    void update(const Utilization &x) {
        _eeprom.update_file(file_util, x);
    }
    void update(const Counters1 &x) {
        _eeprom.update_file(file_cntrs1, x);
    }
    void update(const Counters2 &x) {
        _eeprom.update_file(file_cntrs2, x);
    }
    void update(const TempSensor &x) {
        _eeprom.update_file(file_tempsensor, x);
    }

    Storage() {
        _eeprom.read_file(file_config, config);
        _eeprom.read_file(file_tray, tray);
        _eeprom.read_file(file_util, utlz);
        _eeprom.read_file(file_cntrs1, cntr1);
        _eeprom.read_file(file_cntrs2, cntr2);
        _eeprom.read_file(file_tempsensor, temp);
    }


protected:
    EEPROM<sizeof(StorageSector),file_directory_len> _eeprom;


};


}
