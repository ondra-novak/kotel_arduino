#pragma once
#ifndef SRC_KOTEL_CONTROLLER_CONFIG_H_
#define SRC_KOTEL_CONTROLLER_CONFIG_H_

namespace kotel {


template <typename T>
struct member_pointer_type;

template <typename X, typename Y>
struct member_pointer_type<X Y::*> {
    using type = X;
};

template <typename T>
using member_pointer_type_t = typename member_pointer_type<T>::type;



template<typename X>
void print_data(Stream &s, const X &data) {
    s.print(data);
}


template<>
void print_data(Stream &s, const OneDecimalValue &data) {
    s.print(static_cast<float>(data.value)*0.1f);
}


template<>
void print_data(Stream &s, const SimpleDallasTemp::Address &data) {
    if (data[0]<16) s.print('0');
    s.print(data[0], HEX);
    for (unsigned int i = 1; i < data.size();++i) {
        s.print('-');
        if (data[i]<16) s.print('0');
        s.print(data[i],HEX);
    }
}

template<>
void print_data(Stream &s, const IPAddr &data) {
    s.print(data.ip[0]);
    for (unsigned int i = 1; i < 4; ++i) {
        s.print('.');
        s.print(data.ip[i]);
    }
}

template<>
void print_data(Stream &s, const TextSector &data) {
    auto txt = data.get();
    s.write(txt.data(), txt.size());
}

template<>
void print_data(Stream &s, const PasswordSector &data) {
    auto txt = data.get();
    for (std::size_t i = 0; i < txt.size(); ++i) s.print('*');
}

template<>
void print_data(Stream &s, const std::optional<float> &data) {
    if (data.has_value()) s.print(*data);
}


template<typename T>
void print_data_line(Stream &s, const char *name, const T &object) {
    s.print(name);
    s.print('=');
    print_data(s,object);
    s.print('\n');

}

template<typename Table, typename Object>
void print_table(Stream &s, const Table &table, const Object &object, std::string_view prefix = {}) {
    for (const auto &[k,ptr]: table) {
        if (!prefix.empty()) s.write(prefix.data(), prefix.size());
        print_data_line(s, k, object.*ptr);
    }

}


template<typename UINT>
bool parse_to_field_uint(UINT &fld, std::string_view value) {
    uint32_t v = 0;
    constexpr uint32_t max = ~static_cast<UINT>(0);
    for (char c: value) {
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
        if (v > max) return false;
    }
    fld  = static_cast<UINT>(v);
    return true;
}

bool parse_to_field(int8_t &fld, std::string_view value) {
    uint8_t x;
    if (!value.empty() && value.front() == '-') {
        if (!parse_to_field_uint(x, value.substr(1))) return false;
        fld = -static_cast<int8_t>(x);
        return true;
    } else {
        if (!parse_to_field_uint(x,value)) return false;
        fld = static_cast<int8_t>(x);
        return true;
    }

}

bool parse_to_field(uint8_t &fld, std::string_view value) {
    return parse_to_field_uint(fld,value);
}
bool parse_to_field(uint16_t &fld, std::string_view value) {
    return parse_to_field_uint(fld,value);
}
bool parse_to_field(uint32_t &fld, std::string_view value) {
    return parse_to_field_uint(fld,value);
}

bool parse_to_field(float &fld, std::string_view value) {
    char buff[21];
    char *c;
    value = value.substr(0,20);
    *std::copy(value.begin(), value.end(), buff) = 0;
    fld = std::strtof(buff,&c);
    return c > buff;
}

bool parse_to_field(SimpleDallasTemp::Address &fld, std::string_view value) {
    for (auto &x: fld) {
        auto part = split(value,"-");
        if (part.empty()) return false;
        int v = 0;
        for (char c: part) {
            v = v * 16;
            if (c >= '0' && c <= '9') v += (c-'0');
            else if (c >= 'A' && c <= 'F') v += (c-'A'+10);
            else if (c >= 'a' && c <= 'f') v += (c-'a'+10);
            else return false;
            if (v > 0xFF) return false;
        }
        x = static_cast<uint8_t>(v);
    }
    return (value.empty());
}
bool parse_to_field(IPAddr &fld, std::string_view value) {
    for (auto &x:fld.ip) {
        auto part = split(value,".");
        if (part.empty()) return false;
        if (!parse_to_field(x, part)) return false;
    }
    return true;
}

bool parse_to_field(TextSector &fld, std::string_view value) {
    fld.set_url_dec(value);
    return true;
}

bool parse_to_field(OneDecimalValue &fld, std::string_view value) {
    float f;
    if (!parse_to_field(f, value)) return false;
    fld.value = static_cast<uint8_t>(f * 10.0f);
    return true;
}


template<typename Table, typename Config>
bool update_settings(const Table &table, Config &config, std::string_view key, std::string_view value, std::string_view prefix = {}) {
    if (!prefix.empty()) {
        if (key.substr(0,prefix.size()) == prefix) {
            key = key.substr(prefix.size());
        } else {
            return false;
        }
    }
    for (const auto &[k,ptr]: table) {
        if (k == key) {
            return parse_to_field(config.*ptr, value);
        }
    }
    return false;
}

template<typename Config, typename ... Table>
bool update_settings_kv(Config &config, std::string_view keyvalue, const Table &...tables) {
    std::string tmp;
    auto key = split(keyvalue,"=");
    auto value = keyvalue;
    if (value.find('%') != value.npos) {
        url_decode(value.begin(), value.end(), std::back_inserter(tmp));
        value = tmp;
    }
    key = trim(key);
    value = trim(value);
    return ( update_settings(tables, config, key, value)||...);
}
template<typename Config, typename ... Table>
bool update_settings_fd(Config &config, std::string_view formdata, const Table &...tables) {
    std::string tmp;
    while (!formdata.empty()) {
        auto kv = split(formdata, "&");
        if (!update_settings_kv(config, kv, tables...)) return false;
    }
    return true;
}


}





#endif /* SRC_KOTEL_CONTROLLER_CONFIG_H_ */
