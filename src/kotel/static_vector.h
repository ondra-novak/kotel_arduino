#pragma once

#include <new>

template<typename T, unsigned int N>
class StaticVector {
public:


    StaticVector() = default;
    ~StaticVector() {
        for (T &x: *this) x.~T();
    }
    const T *begin() const {return reinterpret_cast<const T *>(_data);}
    const T *end() const {return reinterpret_cast<const T *>(_data)+_sz;}
    T *begin()  {return reinterpret_cast<T *>(_data);}
    T *end() {return reinterpret_cast<T *>(_data)+_sz;}
    T &operator[](unsigned int idx) {return *(begin()+idx);}
    bool push_back(const T &x) {
        if (_sz >= N) return false;
        new(end()) T(x);
        ++_sz;
        return true;
    }
    bool push_back(T &&x) {
        if (_sz >= N) return false;
        new(end()) T(std::move(x));
        ++_sz;
        return true;
    }
    template<typename ... Args>
    bool emplace_back(Args && ... args) {
        if (_sz >= N) return false;
        new(end()) T(std::forward<Args>(args)...);
        ++_sz;
        return true;
    }

    const T &back() const {return *(end()-1);}
    T &back() {return *(end()-1);}
    const T &front() const {return *(begin());}
    T &front() {return *(end());}

protected:
    char _data[N*sizeof(T)];
    std::size_t _sz = 0;
};
