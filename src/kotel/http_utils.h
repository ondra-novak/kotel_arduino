#pragma once
#include <cctype>
#include <string_view>

namespace kotel {

///Split string into two separated by a separator
/**
 * @param data input and output string. This variable changed to contains other part of the string
 * @param sep separator
 * @return first part part of original string
 *
 * @note if separator is not in the string, returns first argument and variable on
 * first argument is set to ""
 * @note if called with empty string, an empty string is returned
 */
inline  std::string_view split(std::string_view &data, std::string_view sep) {
    std::string_view r;
    auto pos = data.find(sep);
    if (pos == data.npos) {
        r = data;
        data = {};
    } else {
        r = data.substr(0,pos);
        data = data.substr(pos+sep.size());
    }
    return r;
}

///Trim whitespaces at the beginning and at the end of the string
/**
 * @param data string to trim
 * @return trimmed string
 */
inline constexpr std::string_view trim(std::string_view data) {
    while (!data.empty() && isspace(data.front())) data = data.substr(1);
    while (!data.empty() && isspace(data.back())) data = data.substr(0, data.size()-1);
    return data;
}


///Fast convert upper case to lower case.
/**
 * Only works with ASCII < 128. No locales
 * @param c character
 * @return lower character
 */
inline constexpr char fast_tolower(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    else return c;
}

///Compare two strings case insensitive (ASCII only)
/**
 * @param a first string
 * @param b second string
 * @retval true strings are same (case insensitive - ASCII only)
 * @retval false not same
 */
inline constexpr bool icmp(const std::string_view &a, const std::string_view &b) {
    if (a.size() != b.size()) return false;
    std::size_t cnt = a.size();
    for (std::size_t i = 0; i < cnt; ++i) {
        if (fast_tolower(a[i]) != fast_tolower(b[i])) return false;
    }
    return true;
}

struct IStrEqual {
    bool operator()(const std::string_view &a, const std::string_view &b) const{
        return icmp(a,b);
    }
};


///Compare two strings case insensitive (ASCII only)
/**
 * @param a first string
 * @param b second string
 * @retval true strings are same (case insensitive - ASCII only)
 * @retval false not same
 */
inline constexpr bool iless(const std::string_view &a, const std::string_view &b) {
    std::size_t cnt = std::min(a.size(),b.size());
    for (std::size_t i = 0; i < cnt; ++i) {
        if (fast_tolower(a[i]) != fast_tolower(b[i])) {
            return fast_tolower(a[i]) < fast_tolower(b[i]);
        }
    }
    return a.size() < b.size();
}


struct IStrLess {
    bool operator()(const std::string_view &a, const std::string_view &b) const {
        return iless(a,b);
    }
};

struct IStrGreater {
    bool operator()(const std::string_view &a, const std::string_view &b) const {
        return iless(b,a);
    }
};

///Parse a http header
/**
 * @param hdr header data
 * @param cb a callback function, which receives key and value for every header line
 * @return string contains first line of the header
 */
template<typename CB>
inline constexpr std::string_view parse_http_header(std::string_view hdr, CB &&cb) {
    static_assert(std::is_invocable_v<CB, std::string_view, std::string_view>);
    auto first_line = split(hdr, "\r\n");
    while (!hdr.empty()) {
        auto value = split(hdr, "\r\n");
        auto key = split(value,":");
        key = trim(key);
        value = trim(value);
        cb(key, value);
    }
    return first_line;
}

///Parse http header and store result to output iterator
/**
 * @param hdr header
 * @param iter output iterator
 * @return first line
 */
template<typename Iter, typename = std::enable_if<
            std::is_convertible_v<
                std::pair<std::string_view, std::string_view>,
                typename std::iterator_traits<Iter>::value_type>
        > >
inline constexpr std::string_view parse_http_header(std::string_view hdr, Iter &iter) {
    return parse_http_header(hdr, [&](const std::string_view &a, const std::string_view &b){
       *iter += std::pair<std::string_view, std::string_view>(a,b);
    });
}

///Parse http header and store result to output iterator
/**
 * @param hdr header
 * @param iter output iterator
 * @return first line
 */
template<typename Iter, typename = std::enable_if<
        std::is_convertible_v<
            std::pair<std::string_view, std::string_view>,
            typename std::iterator_traits<Iter>::value_type>
    > >
inline constexpr std::string_view parse_http_header(std::string_view hdr, Iter &&iter) {
    return parse_http_header(hdr, iter);
}

enum class HttpMethod {
    GET, POST, PUT, HEAD, OPTIONS, TRACE, DELETE, CONNECT, unknown,
    WS //< used to identify websocket request
};

constexpr std::pair<HttpMethod, const char *> method_map[] = {
        {HttpMethod::GET,"GET"},
        {HttpMethod::POST,"POST"},
        {HttpMethod::PUT,"PUT"},
        {HttpMethod::HEAD,"HEAD"},
        {HttpMethod::OPTIONS,"OPTIONS"},
        {HttpMethod::TRACE,"TRACE"},
        {HttpMethod::DELETE,"DELETE"},
        {HttpMethod::CONNECT,"CONNECT"},
};

struct HttpRequestLine {
    HttpMethod method;
    std::string_view path;
    std::string_view version;
};

///Parse first request line
inline  HttpRequestLine parse_http_request_line(std::string_view first_line) {
    auto m = split(first_line, " ");
    auto p = split(first_line, " ");
    auto v = first_line;
    HttpMethod meth = HttpMethod::unknown;
    for (const auto &[k,v]: method_map) {
        if (icmp(v, m)) {meth = k; break;}
    }
    return {meth,p,v};
}



template <typename InputIt, typename OutputIt>
inline constexpr OutputIt url_encode(InputIt beg, InputIt end, OutputIt result) {
    constexpr char hex_chars[] = "0123456789ABCDEF";

    constexpr auto is_unreserved = [](char c) {
        std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '.' || c == '_' || c == '~';
    };

    for (auto it = beg; it != end; ++it) {
        char c = *it;
        if (is_unreserved(c)) {
            *result++ = c;  // Unreserved znaky přidáme přímo
        } else {
            *result++ = '%';  // Nepovolený znak enkódujeme
            *result++ = hex_chars[(c >> 4) & 0xF];  // Vyšší 4 bity
            *result++ = hex_chars[c & 0xF];         // Nižší 4 bity
        }
    }

    return result;
}


template <typename InputIt, typename OutputIt>
inline constexpr OutputIt url_decode(InputIt beg, InputIt end, OutputIt result) {

    constexpr auto from_hex = [] (char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 2;
    };


    for (auto it = beg; it != end; ++it) {
        char c = *it;
        if (c == '%') {
            ++it;
            if (it == end) break;
            char hex1 = *it;
            ++it;
            if (it == end) break;
            char hex2 = *it;
            *result++ = (from_hex(hex1) << 4) + from_hex(hex2);
        } else {
            *result++ = c;
        }
    }

    return result;
}


}
