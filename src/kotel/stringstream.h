#pragma once


namespace kotel {

template<std::size_t size>
class StringStream: public Stream {
public:

    std::string_view get_text() const {return {_buffer,_sz};}

    void clear() {_sz =0;}

    using Stream::write;

    virtual size_t write(uint8_t x) override {
        if (_sz < size) {
            _buffer[_sz] = x;
            ++_sz;
            return 1;
        }
        return 0;

    }
    virtual int available() override {return 0;};
    virtual int read() override {return -1;};
    virtual int peek() override {return -1;};


protected:
    char _buffer[size];
    std::size_t _sz = 0;
};

class StringStreamExt: public Stream {
public:

    StringStreamExt(char *buffer, std::size_t sz)
        :_buffer(buffer)
        ,_capacity(sz)
        ,_sz(0) {}


    std::string_view get_text() const {return {_buffer,_sz};}

    void clear() {_sz =0;}

    virtual size_t write(uint8_t x) override {
        if (_sz < _capacity) {
            _buffer[_sz] = x;
            ++_sz;
            return 1;
        }
        return 0;

    }
    virtual int available() override {return 0;};
    virtual int read() override {return -1;};
    virtual int peek() override {return -1;};


protected:
    char *_buffer;
    std::size_t _capacity;
    std::size_t _sz = 0;
};

class CountingStream: public Stream {
public:

    std::size_t get_count() const {return _sz;}

    virtual size_t write(uint8_t ) override {++_sz;return 1;}
    virtual int available() override {return 0;};
    virtual int read() override {return -1;};
    virtual int peek() override {return -1;};

protected:
    std::size_t _sz = 0;
};

}

