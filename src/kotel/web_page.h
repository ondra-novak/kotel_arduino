#pragma once

#include "base64.h"

class ETagCalc {
public:
    static constexpr unsigned int need_sz = sizeof(uint32_t) * 2+2;
    
 
    template<typename X>
    static constexpr uint32_t fnv1a_hash_32(const X& str) {
        constexpr uint32_t fnv_offset_basis = 2166136261u;
        constexpr uint32_t fnv_prime = 16777619u;

        uint32_t hash = fnv_offset_basis;
        for (char c : str) {
            hash ^= static_cast<uint32_t>(c);
            hash *= fnv_prime;
        }
        return hash;
    }
    
    template<typename X>
    constexpr ETagCalc(const X& data) {
        uint32_t h = fnv1a_hash_32(data);
        _etag_data[0] = '"';
        _etag_data[need_sz-1] = '"';
        for (int i = need_sz - 2; i > 0; --i) {
            auto p = h & 0xF;
            h = h >> 4;
            _etag_data[i] = p<10?('0'+p):('A'+p-10);
        }
    }
    constexpr operator std::string_view() const {return {_etag_data, need_sz};}
    
protected:
    char _etag_data[need_sz] = {};   
        
};

constexpr auto embedded_index_html = binary_data("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");
constexpr auto embedded_index_html_etag = ETagCalc(embedded_index_html);

