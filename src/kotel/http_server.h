#pragma once
#include "http_utils.h"
#include <WiFiS3.h>
#include "print_hlp.h"
#include "combined_container.h"
#include "stringstream.h"

namespace kotel {



class HttpServerBase {
public:

    using HeaderPair = std::pair<std::string_view, std::string_view>;
    using HeaderIL = std::initializer_list<HeaderPair>;

    struct Request {
        WiFiClient client = {};
        HttpRequestLine request_line = {};
        const std::pair<std::string_view, std::string_view>  *headers = {};
        std::size_t headers_count = {};
        std::string_view body = {};
    };


    constexpr static std::string_view get_message(int code) {
        switch(code) {
            case 200: return "OK";
            case 202: return "Accepted";
            case 400: return "Bad request";
            case 404: return "Not found";
            case 405: return "Method not allowed";
            case 409: return "Conflict";
            default: return "Unknown error";
        }
    }

    struct ContentType {
        static constexpr char text[]= "text/plain;charset=utf-8";
        static constexpr char html[]= "text/html;charset=utf-8";
        static constexpr char json[]= "application/json";
        static constexpr char css[]= "text/css";
        static constexpr char javascript[]= "text/javascript";
        static constexpr char png[]= "image/png";
        static constexpr char gif[]= "image/gif";
        static constexpr char jpeg[]= "image/jpeg";
    };


};


template<unsigned int max_request_size = 8192,
         unsigned int max_header_lines = 32>
class HttpServer: public HttpServerBase {
public:

    HttpServer(int port);
    void begin();


    Request get_request();


    ///Send header
    /**
     * @param req request
     * @param header container which contains header lines, std::pair<string, string>
     * @param code status code
     * @param message status message
     *
     * @note reuses server's buffer. You should avoid to reference any string
     * retrieved by request. It also destroys the request content
     */
    template<typename KeyValueHeader>
    void send_header(Request &req,
            const KeyValueHeader &header,
            int code = 200, std::string_view message = "");

    ///Send header
    /**
     * @param req request
     * @param header container which contains header lines, std::pair<string, string>
     * @param code status code
     * @param message status message
     *
     * @note reuses server's buffer. You should avoid to reference any string
     * retrieved by request. It also destroys the request content
     */
    void send_header(Request &req,
            const HeaderIL &header,
            int code = 200, std::string_view message = "");

    ///Send error response
    /**
     * @param req
     * @param code
     * @param message
     * @param extra_headers
     * @param extra_message
     *
     * @note reuses server's buffer. You should avoid to reference any string
     * retrieved by request. It also destroys the request content
     */
    void error_response(Request &req,
            int code,
            std::string_view message,
            const HeaderIL &extra_headers = {},
            std::string_view extra_message = {});

    ///send simple header for serve a content
    /**
     * @param req request
     * @param content_type content type
     * @param content_len content length, if -1 then Connection: close is added
     * @note reuses server's buffer. You should avoid to reference any string
     * retrieved by request. It also destroys the request content
     */
    void send_simple_header(Request &req, std::string_view content_type, int content_len = -1);

    ///send file
    /**
     * @param req request
     * @param content_type content type
     * @param content content
     */
    void send_file(Request &req, std::string_view content_type, std::string_view content);

protected:
    WiFiServer _srv = {};
    WiFiClient _active_client = {};
    char _input_buff[max_request_size] = {};
    unsigned int _write_pos = 0;
    unsigned int _hdr_end = 0;
    unsigned int _hdr_count = 0;
    HttpRequestLine _rl;
    int _body_size = -1;    //-1 reading header, 0 = no body, else size of body
    bool _body_trunc = false;
    unsigned long read_timeout_tp = 0;
    std::pair<std::string_view, std::string_view>  _header_lines[max_header_lines];

    void parse_header();
    void reset_server();
};




template<unsigned int max_request_size, unsigned int max_header_lines >
inline HttpServer<max_request_size, max_header_lines>::HttpServer(int port)
    :_srv(port)
{

}

template<unsigned int max_request_size, unsigned int max_header_lines>
inline void HttpServer<max_request_size, max_header_lines>::begin() {
    _srv.begin();
}

template<unsigned int max_request_size, unsigned int max_header_lines>
inline typename  HttpServer<max_request_size, max_header_lines>::Request
    HttpServer<max_request_size, max_header_lines>::get_request() {

    Request ret {};
    auto curtm = millis();
    if (static_cast<long>(curtm - read_timeout_tp) > 0 && _active_client) {
        _active_client.stop();
        reset_server();
        return ret;
    }

    if (!_active_client) {
        _active_client = _srv.available();
        if (!_active_client) {
            return ret;
        }
        read_timeout_tp = curtm+5000;    //total timeout
    }
    int b = _active_client.read();
    while (b != -1) {
        _input_buff[_write_pos] = static_cast<char>(b);
        ++_write_pos;
        if (_body_size == -1) {
            if (_write_pos > 3
                && _input_buff[_write_pos-1] == '\n'
                && _input_buff[_write_pos-2] == '\r'
                && _input_buff[_write_pos-3] == '\n'
                && _input_buff[_write_pos-4] == '\r') {
                    _write_pos-=2;  //save 2 bytes for body (empty header line)
                    _hdr_end = _write_pos;
                    parse_header();
                    if (_rl.version.empty()) {
                        _active_client.stop();
                        reset_server();
                        return ret;
                    }
            } else if (_write_pos == max_request_size) {
                _active_client.stop();
                reset_server();
                return ret;
            }
        } else {
            --_body_size;
            if (_write_pos == max_request_size) {
                --_write_pos;
                _body_trunc = true;
            }
        }
        if (_body_size == 0) {
            ret.client = _active_client;
            ret.request_line = _rl;
            ret.headers = _header_lines;
            ret.headers_count = _hdr_count;
            ret.body = {_input_buff+_hdr_end, _write_pos -_hdr_end};
            if (_body_trunc) {
                reset_server();
                error_response(ret, 413, "Content Too Large");
                ret.client.stop();
                ret = {};
                return ret;
            } else {
                reset_server();
                return ret;
            }
        }
        b = _active_client.read();
    }
    return ret;
}


template<typename KeyValueHeader>
inline void send_header_impl(Stream &s,
        std::string_view ver,
        int code,
        std::string_view message,
        const KeyValueHeader &header) {

    print(s,ver, " ",code, " ", message,"\r\n");
    bool keep_alive = false;
    for (const auto &[k, v]: header) {
        print(s, k, ": ", v, "\r\n");
        keep_alive = keep_alive || icmp(k, "Content-Length");
    }
    if (!keep_alive) {
        print(s, "Connection: close\r\n");
    }
    s.print("\r\n");
}



template<unsigned int max_request_size, unsigned int max_header_lines>
inline void HttpServer<max_request_size, max_header_lines>::error_response(
        Request &req,
        int code,
        std::string_view message,
        const HeaderIL &extra_header,
        std::string_view extra_message) {

    if (message.empty()) message = get_message(code);
    StringStreamExt buff(_input_buff, max_request_size);

    std::initializer_list<std::pair<std::string_view, std::string_view> > hdr = {
            {"Content-Type","text/html; charset=utf-8"}
    };
    send_header_impl(buff, req.request_line.version, code, message,
            CombinedContainers<HeaderIL,HeaderIL>(hdr, extra_header));

    print(buff, "<!DOCTYPE html><html><head><title>",
            code," ",message,"</title></head><body><h1>",
            code," ",message,"</h1><pre>",extra_message,"</pre></body></html>");
    auto data = buff.get_text();
    req.client.write(data.data(), data.size());


}

template<unsigned int max_request_size, unsigned int max_header_lines>
inline void HttpServer<max_request_size, max_header_lines>::parse_header() {
    _body_size = 0;
    auto first_line = parse_http_header(std::string_view(_input_buff, _hdr_end),
            [&](std::string_view key, std::string_view value){
        if (icmp(key, "Content-Length")) {
            _body_size = std::strtoul(value.data(), nullptr, 10);
        }
        if (_hdr_count != max_header_lines) {
            _header_lines[_hdr_count] = {key,value};
            ++_hdr_count;
        }
    });
    _rl = parse_http_request_line(first_line);
}

template<unsigned int max_request_size, unsigned int max_header_lines>
inline void HttpServer<max_request_size, max_header_lines>::reset_server() {
    _body_size = -1;
    _active_client = {};
    _write_pos = 0;
    _hdr_end = 0;
    _hdr_count = 0;
    _body_trunc = false;
}

template<unsigned int max_request_size, unsigned int max_header_lines>
template<typename KeyValueHeader>
inline void HttpServer<max_request_size, max_header_lines>::send_header(Request &req,
        const KeyValueHeader &header,
        int code, std::string_view message) {

    if (message.empty()) message = get_message(code);
    StringStreamExt buff(this->_input_buff, max_request_size);
    send_header_impl(buff, req.request_line.version, code, message, header);
    auto txt = buff.get_text();
    req.client.write(txt.data(), txt.size());

}

template<unsigned int max_request_size, unsigned int max_header_lines>
inline void HttpServer<max_request_size, max_header_lines>::send_header(Request &req,
        const HeaderIL &header,
        int code, std::string_view message) {
    send_header<decltype(header)>(req, header, code, message);
}

template<unsigned int max_request_size, unsigned int max_header_lines>
void HttpServer<max_request_size, max_header_lines>::send_simple_header(Request &req, std::string_view content_type, int content_len) {
    HeaderPair hp[2];
    char buff[20];
    char *c = std::end(buff);
    *(--c) = 0;
    hp[0].first = "Content-Type";
    hp[0].second = content_type;
    if (content_len < 0) {
        send_header(req, {hp[0]}, 200, {});
    } else {
        hp[1].first = "Content-Length";
        if (content_len == 0) *(--c) = '0';
        else while (content_len != 0) {
            *(--c) = (content_len % 10) + '0';
            content_len/=10;
        }
        hp[1].second = c;
        send_header(req, {hp[0],hp[1]}, 200, {});
    }
}

template<unsigned int max_request_size, unsigned int max_header_lines>
void HttpServer<max_request_size, max_header_lines>::send_file(Request &req, std::string_view content_type, std::string_view content) {
    send_simple_header(req, content_type, content.size());
    req.client.write(content.data(), content.size());
}




}
