#pragma once


template<typename Cont1, typename Cont2>
class CombinedContainers {
public:

    CombinedContainers(const Cont1 &cont1, const Cont2 &cont2)
        :_cont1(cont1)
        ,_cont2(cont2) {}

    using Iter1 = decltype(std::declval<Cont1>().begin());
    using Iter2 = decltype(std::declval<Cont2>().begin());

    class Iterator {
    public:

        using value_type = typename Cont1::value_type;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        Iterator(Iter1 iter1, Iter1 end1, Iter2 iter2)
            :_iter1(iter1),_end1(end1),_iter2(iter2) {}

        const value_type &operator *() const {
            if (_iter1 == _end1) return *_iter2;
            else return *_iter1;
        }
        const value_type *operator ->() const {
            return &(*(*this));
        }

        bool operator==(const Iterator &other) const {
            if (_iter1 == _end1 && other._iter1 == other._end1) {
                return _iter2 == other._iter2;
            } else {
                return _iter1 == other._iter1;
            }
        }

        bool operator!=(const Iterator &other) const {
            return !operator==(other);
        }
        Iterator &operator++() {
            if (_iter1 == _end1) ++_iter2;
            else ++_iter1;
            return *this;
        }

        Iterator operator++(int) {
            Iterator save = *this;
            ++(*this);
            return save;
        }


    protected:
        Iter1 _iter1;
        Iter1 _end1;
        Iter2 _iter2;
    };

    Iterator begin() const {
        return {_cont1.begin(), _cont1.end(), _cont2.begin()};
    }
    Iterator end() const {
        return {_cont1.end(), _cont1.end(), _cont2.end()};
    }

protected:
    const Cont1 &_cont1;
    const Cont2 &_cont2;

};
