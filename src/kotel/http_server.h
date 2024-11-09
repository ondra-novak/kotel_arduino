#pragma once
#include "http_utils.h"
#include <WiFiS3.h>

namespace kotel {

template<unsigned int max_request_size = 8192,
         unsigned int max_header_lines = 32>
class HttpServer {
public:

    HttpServer(int port);
    void begin();

    struct Request {
        WiFiClient client = {};
        HttpRequestLine request_line = {};
        const std::pair<std::string_view, std::string_view>  *headers = {};
        std::size_t headers_count = {};
        std::string_view body = {};
    };

    Request get_request();


    void error_response(const Request &req,
            int code,
            std::string_view message,
            std::string_view extra_headers = {},
            std::string_view extra_message = {});




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
        reset_server();
        _active_client.stop();
        _active_client = {};
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
            } else if (_write_pos == max_request_size) {
                _active_client.stop();
                _active_client = {};
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



template<unsigned int max_request_size, unsigned int max_header_lines>
inline void HttpServer<max_request_size, max_header_lines>::error_response(
        const Request &req,
        int code,
        std::string_view message,
        std::string_view extra_headers,
        std::string_view extra_message) {

    WiFiClient client = req.client;
    auto emit_msg = [&] {
        client.print(code);
        client.print(' ');
        client.write(message.data(), message.size());

    };
    client.write(req.request_line.version.data(),req.request_line.version.size());
    client.print(' ');
    emit_msg();
    client.println();
    client.println("Connection: close");
    client.println("Content-Type: text/html;charset=utf-8");
    client.write(extra_headers.data(), extra_headers.size());
    client.println();
    client.print("<!DOCTYPE html><html><head><title>");
    emit_msg();
    client.print("</title></head><body><h1>");
    emit_msg();
    client.print("</h1><pre>");
    client.write(extra_message.data(), extra_message.size());
    client.print("</pre></body></html>");


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

}
