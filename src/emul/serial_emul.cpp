#include "api/Common.h"
#include "serial_emul.h"
#include <queue>
#include <string_view>


UART Serial;

static std::queue<char> _uart_queue = {};

static std::vector<char> cur_buffer = {};
static std::vector<char> prev_buffer = {};

void uart_output(std::string_view text) {
    cur_buffer.insert(cur_buffer.end(),text.begin(), text.end());
}

std::string_view uart_output() {
    std::swap(cur_buffer, prev_buffer);
    cur_buffer.clear();
    return {prev_buffer.data(), prev_buffer.size()};
}

void uart_input(std::string_view text) {
    for (auto c: text) _uart_queue.push(c);
    _uart_queue.push('\n');
}

void UART::begin(unsigned long) {}

void UART::begin(unsigned long, uint16_t) {}

void UART::end() {}


int UART::available(void) {
    return _uart_queue.empty()?0:1;
}

int UART::peek(void) {
    if (_uart_queue.empty()) return -1;
    return _uart_queue.front();
}

int UART::read(void) {
    if (_uart_queue.empty()) return -1;
    int r = _uart_queue.front();
    _uart_queue.pop();
    return r;
}

void UART::flush(void) {
    while (available()) {
        read();
    }
}

size_t UART::write(uint8_t c) {
    uart_output({reinterpret_cast<const char *>(&c),1});
    return 1;
}

size_t UART::write(uint8_t* c, size_t len) {
    uart_output({reinterpret_cast<const char *>(c), len});
    return len;
}

size_t UART::write_raw(uint8_t* c, size_t len) {
    return write(c, len);
}

UART::operator bool()  { return true; }
