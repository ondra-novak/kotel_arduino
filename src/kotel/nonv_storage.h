#pragma once
#include "nonv_storage_def.h"
#include <r4eeprom.h>

#include <algorithm>
namespace kotel {

struct TextSector {
    char text[eeprom_sector_size] = {};
    void set(std::string_view txt) {
        auto b = txt.begin();
        auto e = txt.end();
        for (char &c: text) {
            if (b == e) {
                c = '\0';
            } else {
                c = *b;
                ++b;
            }
        }
    }

    static char from_hex_digit(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A';
        if (c >= 'a' && c <= 'f') return c - 'a';
        return 2;
    }

    void set_url_dec(std::string_view txt) {
        auto b = txt.begin();
        auto e = txt.end();
        for (char &c: text) {
            if (b == e) {
                c = '\0';
            } else {
                c = *b;
                ++b;
                if (c == '%' && b != e) {
                    c = from_hex_digit(*b);
                    ++b;
                    if (b != e) {
                        c = (c << 4) | from_hex_digit(*b);
                    }
                    ++b;
                }
            }
        }
    }
    std::string_view get() const {
        auto iter = std::find(std::begin(text), std::end(text), '\0');
        return std::string_view(text, std::distance(std::begin(text), iter));
    }
};

struct PasswordSector: TextSector {};

struct WiFi_SSID {
    TextSector ssid = {};
};
struct WiFi_Password{
    PasswordSector password = {};
};



class Storage {
public:


    Config config;
    Runtime runtm;
    Counters cntr;
    TempSensor temp;
    Snapshot snapshot;
    Tray tray;
    WeekRecord weeks[file_history_count];
    WiFi_SSID wifi_ssid;
    WiFi_Password wifi_password;
    WiFi_Password pair_secret;
    WiFi_NetSettings wifi_config;
    bool pair_secret_need_init = false;


    void begin() {
        _eeprom.begin();
        _eeprom.read_file(file_config, config);
        _eeprom.read_file(file_snapshot, snapshot);
        _eeprom.read_file(file_tray, tray);
        _eeprom.read_file(file_runtime, runtm);
        _eeprom.read_file(file_cntrs, runtm);
        _eeprom.read_file(file_tempsensor, temp);
        _eeprom.read_file(file_wifi_ssid, wifi_ssid);
        _eeprom.read_file(file_wifi_pwd, wifi_password);
        _eeprom.read_file(file_wifi_net, wifi_config);
        for (auto &x: weeks) {
            int idx = &x - weeks;
            _eeprom.read_file(file_history_offset+idx, x);
        }
        pair_secret_need_init = !_eeprom.read_file(file_pair_secret,pair_secret);
    }
    auto get_eeprom() const {
        return _eeprom;
    }

    ///save change sesttings now
    /** This operation is still asynchronous. The controller must call flush()
     * to perform actual store
     */
    void save() {
        _update_flag = true;
    }

    void commit() {
        if (_update_flag) {
            _eeprom.update_file(file_config,config);
            _eeprom.update_file(file_snapshot,snapshot);
            _eeprom.update_file(file_runtime,runtm);
            _eeprom.update_file(file_cntrs,cntr);
            _eeprom.update_file(file_tempsensor,temp);
            _eeprom.update_file(file_wifi_ssid,wifi_ssid);
            _eeprom.update_file(file_wifi_pwd,wifi_password);
            _eeprom.update_file(file_wifi_net, wifi_config);
            _eeprom.update_file(file_pair_secret, pair_secret);
            _update_flag = false;
        }
    }

    void record_day_stats(uint16_t day) {
        if (snapshot._day_number != day) {
            WeekRecord *t = nullptr;
            WeekRecord *f = nullptr;
            uint16_t v = 0xFFFF;
            for (auto &x:weeks) {
                if (x._day_number + week_stat_day_count > day) {
                    f = &x;
                    break;
                }
                if (x._day_number < v) {
                    t = &x;
                    v = x._day_number;
                }
            }
            if (f == nullptr) {
                f = t;
                f->_day_number = day;
            }

            auto uint16cast = [](uint32_t v) ->uint16_t {
                if (v > 0xFFFE) return 0xFFFE;
                else return static_cast<uint16_t>(v);
            };

            auto ofs = day - f->_day_number;
            f->_feeder[ofs] = uint16cast(runtm.feeder - snapshot._feeder);
            f->_feeder_low[ofs] = uint16cast(runtm.feeder_low - snapshot._feeder_low);
            snapshot._feeder += f->_feeder[ofs];
            snapshot._feeder_low += f->_feeder_low[ofs];
            snapshot._day_number = day;
            _eeprom.update_file(file_history_offset + (f - weeks), *f);
            _eeprom.update_file(file_snapshot, snapshot);
        }
    }

    int32_t calc_remaining_fuel() const {
        int32_t cur_time = tray.feeder_time_accum + (runtm.feeder - tray.feeder_time_last_fill);
        int32_t speed = tray.feeder_speed;
        int32_t remain = tray.initial_fill_adj + tray.feeder_time_accum - cur_time / speed;
        return remain;
    }

    unsigned int calc_remaining_fuel_pct() const {
        int32_t r= calc_remaining_fuel();
        int32_t capacity = static_cast<int32_t>(config.tray_kg);
        return r < 0? 0 : r > capacity? 100 : 100*r/capacity;
    }

    void add_fuel(int kg) {

        int32_t capacity = static_cast<int32_t>(config.tray_kg);
        if (tray.feeder_time_last_fill == 0) {
            tray.feeder_time_last_fill = runtm.feeder;
            tray.initial_fill_adj = kg;
            tray.feeder_time_accum = 0;
            tray.fuel_kg_accum = 0;
        } else {
            int32_t remain = calc_remaining_fuel();
            tray.feeder_time_accum += runtm.feeder - tray.feeder_time_last_fill;
            tray.fuel_kg_accum += kg;
            tray.feeder_time_last_fill = runtm.feeder;
            if (remain < 0) {
                tray.initial_fill_adj -= remain;
                remain = 0;
            }
            if (remain + kg > capacity) {
                tray. initial_fill_adj -= remain + kg - capacity;
            }
            int32_t calc_speed = tray.feeder_time_accum / tray.fuel_kg_accum;
            calc_speed = (calc_speed * 3 + tray.feeder_speed)/4;
            tray.feeder_speed = static_cast<uint16_t>(calc_speed);
        }

        _eeprom.update_file(file_tray, tray);
    }

    void set_max_fill() {
        int32_t capacity = static_cast<int32_t>(config.tray_kg);
        int32_t remain = calc_remaining_fuel();
        add_fuel(std::min(capacity, capacity - remain));
    }


protected:
    EEPROM<eeprom_sector_size,file_directory_len> _eeprom;
    bool _update_flag = false;


};


}

