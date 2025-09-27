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

    struct AllDailyData {
        DailyData feeder;
        DailyData feeder_low;
        DailyData fuel;
        DailyData errors;
        DailyData controls;

    };

    Config config;
    Runtime runtm;
    Counters cntr;
    TempSensor temp;
    Snapshot snapshot;
    Tray tray;
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

    template<typename Fn>
    void manage_day_stats(uint16_t day, Fn &&fn) {
        auto index = (day/eeprom_sector_size)%graph_files;
        auto pos = day %  eeprom_sector_size;
        auto fdofs = index + files_feeder_graph;
        auto fdlofs = index + files_feeder_low_graph;
        auto flofs = index + files_fuel_fill_graph;
        auto errofs = index + files_errors_log;
        auto cntofs = index + files_control_log;

        AllDailyData dd;

        _eeprom.read_file(fdofs, dd.feeder);
        _eeprom.read_file(fdlofs, dd.feeder_low);
        _eeprom.read_file(flofs, dd.fuel);
        _eeprom.read_file(errofs, dd.errors);
        _eeprom.read_file(cntofs, dd.controls);

        if (fn(pos, dd)) {
            _eeprom.update_file(fdofs, dd.feeder);
            _eeprom.update_file(fdlofs, dd.feeder_low);
            _eeprom.update_file(flofs, dd.fuel);
            _eeprom.update_file(errofs, dd.errors);
            _eeprom.update_file(cntofs, dd.controls);
        }

    }

    void record_day_stats(uint16_t day) {
        if (snapshot.day_number == day) return;

        auto build_half = [](uint16_t a, uint16_t b) {
            if (a > 0xE) a = 0xF;
            if (b > 0xE) b = 0xF;
            return (static_cast<uint8_t>(a) << 4) | static_cast<uint8_t>(b);
        };
        auto build_fdr = [](uint32_t a, uint32_t b) -> uint8_t {
            if (a < b) return 0;
            auto diff = a - b;
            diff >>= 7;
            if (diff > 0xFF) diff = 0xFF;
            return static_cast<uint8_t>(diff);
        };
        auto restore_fdr = [](uint32_t a, uint8_t diff) {
            return a + static_cast<uint32_t>(diff << 7);
        };

        //don't clear if brand new
        if (snapshot.day_number) {
            uint16_t to_clear = std::min<uint16_t>(day - (snapshot.day_number+1),360);
            for (uint16_t i = 0; i < to_clear; ) {
                manage_day_stats(snapshot.day_number+i,[&](auto pos, AllDailyData &dd){
                    while (pos < eeprom_sector_size && i < to_clear) {
                        dd.feeder._days[pos] = 0;
                        dd.feeder_low._days[pos] = 0;
                        dd.fuel._days[pos] = 0;
                        dd.errors._days[pos] = 0;
                        dd.controls._days[pos] = 0;
                        ++pos;
                        ++i;
                    }
                    return true;
                });
            }
        }

        manage_day_stats(day, [&](
                auto pos, AllDailyData &dd) {

            dd.feeder._days[pos] = build_fdr(runtm.feeder , snapshot.feeder);
            dd.feeder_low._days[pos] = build_fdr(runtm.feeder_low , snapshot.feeder_low);
            dd.fuel._days[pos] = static_cast<uint8_t>(std::min<uint32_t>(0xFF,tray.fuel_kg_accum - snapshot.fuel_kg));
            dd.errors._days[pos] = build_half(cntr.overheat_count- snapshot.overheat_count,
                                              cntr.therm_failure_count - snapshot.therm_fail_count);
            dd.controls._days[pos] = build_half(cntr.manual_control_count- snapshot.manual_count,
                                              cntr.restart_count - snapshot.restart_count);

            snapshot.day_number = day;
            snapshot.feeder = restore_fdr(snapshot.feeder, dd.feeder._days[pos]);
            snapshot.feeder_low = restore_fdr(snapshot.feeder_low, dd.feeder_low._days[pos]);
            snapshot.fuel_kg = tray.fuel_kg_accum;
            snapshot.manual_count = cntr.manual_control_count;
            snapshot.overheat_count = cntr.overheat_count;
            snapshot.restart_count = cntr.restart_count;
            snapshot.therm_fail_count = cntr.therm_failure_count;

            _eeprom.update_file(file_snapshot, snapshot);
            return true;
        });
    }

    Snapshot read_day_stats(uint16_t day) {
        Snapshot r;
        manage_day_stats(day,[&](int pos,const AllDailyData &dd){

            r.day_number = day;
            r.feeder = static_cast<uint32_t>(dd.feeder._days[pos])<<7;
            r.feeder_low = static_cast<uint32_t>(dd.feeder_low._days[pos])<<7;
            r.fuel_kg = static_cast<uint16_t>(dd.fuel._days[pos]);
            r.manual_count = static_cast<uint16_t>(dd.controls._days[pos]>>4);
            r.restart_count = static_cast<uint16_t>(dd.controls._days[pos] & 0xF);
            r.overheat_count = static_cast<uint16_t>(dd.errors._days[pos]>>4);
            r.therm_fail_count = static_cast<uint16_t>(dd.errors._days[pos] & 0xF);

            return false;
        });
        return r;
    }

    int32_t calc_remaining_fuel() const {
        int32_t cur_time = tray.feeder_time_accum + (runtm.feeder - tray.feeder_time_last_fill);
        int32_t speed = tray.feeder_speed;
        int32_t remain = tray.initial_fill_adj + tray.fuel_kg_accum - cur_time / speed;
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
            tray.initial_fill_adj += kg;
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
            int32_t calc_speed = std::max<int32_t>(1,tray.feeder_time_accum / tray.fuel_kg_accum);
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

