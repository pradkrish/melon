#ifndef MELON_STATIC_MAP_BOOL_HPP
#define MELON_STATIC_MAP_BOOL_HPP

#include <algorithm>
#include <bit>
#include <cassert>
#include <memory>
#include <ranges>
#include <vector>

#include "melon/container/static_map.hpp"
#include "melon/detail/intrusive_view.hpp"

namespace fhamonic {
namespace melon {

template <typename K>
    requires std::integral<K>
class static_map<K, bool> {
public:
    using value_type = bool;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    using span_type = std::size_t;
    static_assert(std::is_unsigned_v<span_type>);
    static constexpr size_type N = sizeof(span_type) << 3;
    static constexpr size_type span_index_mask = N - 1;
    static constexpr size_type nb_spans(std::size_t n) {
        return (n + N - 1) / N;
    }

public:
    // Branchless version
    // class bit_reference {
    // private:
    //     span_type * _p;
    //     size_type _local_index;

    // public:
    //     reference(span_type * p, size_type index)
    //         : _p(p), _local_index(index) {}
    //     reference(const reference &) = default;

    //     operator bool() const noexcept { return (*_p >> _local_index) & 1; }
    //     reference & operator=(bool b) noexcept {
    //         *_p ^= (((*_p >> _local_index) & 1) ^ b) << _local_index;
    //         *_p ^= (*_p & ~(static_cast<span_type>(b) << _local_index)) |
    //         (*_p & (static_cast<span_type>(b) << _local_index)); return
    //         *this;
    //     }
    //     reference & operator=(const reference & other) noexcept {
    //         return *this = bool(other);
    //     }
    //     bool operator==(const reference & x) const noexcept {
    //         return bool(*this) == bool(x);
    //     }
    //     bool operator<(const reference & x) const noexcept {
    //         return !bool(*this) && bool(x);
    //     }
    // };

    class bit_reference {
    private:
        span_type * _p;
        span_type _mask;

    public:
        reference(span_type * x, size_type y)
            : _p(x), _mask(span_type(1) << y) {}
        reference() noexcept : _p(0), _mask(0) {}
        reference(const reference &) = default;

        operator bool() const noexcept { return !!(*_p & _mask); }
        reference & operator=(bool x) noexcept {
            if(x)
                *_p |= _mask;
            else
                *_p &= ~_mask;
            return *this;
        }
        reference & operator=(const reference & x) noexcept {
            return *this = bool(x);
        }
        bool operator==(const reference & x) const {
            return bool(*this) == bool(x);
        }
        bool operator<(const reference & x) const {
            return !bool(*this) && bool(x);
        }
    };
    using const_reference = bool;

    template <typename I>
    class iterator_base {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type = static_map<bool>::difference_type;
        using value_type = bool;
        using pointer = void;

    protected:
        span_type * _p;
        size_type _local_index;

    public:
        iterator_base(span_type * p, size_type index)
            : _p(p), _local_index(index) {}

        iterator_base() = default;
        iterator_base(const iterator_base &) = default;
        iterator_base(iterator_base &&) = default;

        iterator_base & operator=(const iterator_base &) = default;
        iterator_base & operator=(iterator_base &&) = default;

    protected:
        constexpr void _incr() noexcept {
            if(_local_index++ == N - 1) {
                ++_p;
                _local_index = 0;
            }
        }
        constexpr void _decr() noexcept {
            if(_local_index++ == 0) {
                --_p;
                _local_index = N - 1;
            }
        }
        constexpr void _incr(difference_type i) noexcept {
            difference_type n = static_cast<difference_type>(_local_index) + i;
            _p += n / difference_type(N);
            n = n % difference_type(N);
            if(n < 0) {
                n += difference_type(N);
                --_p;
            }
            _local_index = static_cast<size_type>(n);
        }

    public:
        friend bool operator==(const iterator_base & x,
                               const iterator_base & y) noexcept {
            return x._p == y._p && x._local_index == y._local_index;
        }
        friend constexpr std::strong_ordering operator<=>(
            const iterator_base & x, const iterator_base & y) noexcept {
            if(const auto cmp = x._p <=> y._p; cmp != 0) return cmp;
            return x._local_index <=> y._local_index;
        }
        difference_type operator-(const iterator_base & other) const noexcept {
            return (difference_type(N) * (_p - other._p) +
                    static_cast<difference_type>(_local_index) -
                    static_cast<difference_type>(other._local_index));
        }
        I & operator++() noexcept {
            _incr();
            return *static_cast<I *>(this);
        }
        I operator++(int) noexcept {
            I tmp = *static_cast<I *>(this);
            _incr();
            return tmp;
        }
        I & operator--() noexcept {
            _decr();
            return *static_cast<I *>(this);
        }
        I operator--(int) noexcept {
            I tmp = *static_cast<I *>(this);
            _decr();
            return tmp;
        }
        I & operator+=(difference_type i) noexcept {
            _incr(i);
            return *static_cast<I *>(this);
        }

        I & operator-=(difference_type i) noexcept {
            _incr(-i);
            return *static_cast<I *>(this);
        }

        friend I operator+(const I & x, difference_type n) {
            I tmp = x;
            tmp += n;
            return tmp;
        }
        friend I operator+(difference_type n, const I & x) { return x + n; }
        friend I operator-(const I & x, difference_type n) {
            I tmp = x;
            tmp -= n;
            return tmp;
        }
    };

    class iterator : public iterator_base<iterator> {
    public:
        using iterator_base<iterator>::iterator_base;
        using reference = static_map<bool>::reference;

        reference operator*() const noexcept {
            return reference(_p, _local_index);
        }
        reference operator[](difference_type i) const { return *(*this + i); }
    };

    class const_iterator : public iterator_base<const_iterator> {
    public:
        using iterator_base<const_iterator>::iterator_base;
        using reference = const_reference;

        const_reference operator*() const noexcept {
            return (*_p >> _local_index) & 1;
        }
        const_reference operator[](difference_type i) const {
            return *(*this + i);
        }
    };

private:
    std::unique_ptr<span_type[]> _data;
    size_type _size;

public:
    static_map() : _data(nullptr), _size(0){};
    static_map(size_type size)
        : _data(std::make_unique_for_overwrite<span_type[]>(nb_spans(size)))
        , _size(size){};

    static_map(size_type size, bool init_value) : static_map(size) {
        fill(init_value);
    };

    static_map(const static_map & other) : static_map(other._size) {
        std::copy(other._data.get(), other._data.get() + nb_spans(other._size),
                  _data.get());
    };
    static_map(static_map &&) = default;

    static_map & operator=(const static_map & other) {
        resize(other.size());
        std::copy(other._data.get(), other._data.get() + nb_spans(other._size),
                  _data.get());
        return *this;
    };
    static_map & operator=(static_map &&) = default;

    iterator begin() noexcept { return iterator(_data.get(), 0); }
    iterator end() noexcept {
        return iterator(_data.get() + _size / N, _size & span_index_mask);
    }
    const_iterator begin() const noexcept {
        return const_iterator(_data.get(), 0);
    }
    const_iterator end() const noexcept {
        return const_iterator(_data.get() + _size / N, _size & span_index_mask);
    }

    size_type size() const noexcept { return _size; }
    void resize(size_type n) {
        if(n == _size) return;
        _data = std::make_unique_for_overwrite<span_type[]>(nb_spans(n));
        _size = n;
    }

    reference operator[](size_type i) noexcept {
        assert(i < size());
        return reference(_data.get() + i / N, i & span_index_mask);
    }
    const_reference operator[](size_type i) const noexcept {
        assert(i < size());
        return reference(_data.get() + i / N, i & span_index_mask);
    }

    void fill(bool b) noexcept {
        std::fill(_data.get(), _data.get() + nb_spans(_size),
                  b ? ~span_type(0) : span_type(0));
    }

    auto true_keys() const {
        // span_type * p = _data.get();
        // size_type index = 0;
        // const span_type * p_end = _data.get() + _size / N;
        // const size_type index_end = _size & span_index_mask;

        // for(;;) {
        //     index += static_cast<size_type>(std::countr_zero((*p) >> index));
        //     if(p == p_end && index >= index_end) co_return;
        //     if(index >= N) {
        //         ++p;
        //         index = 0;
        //         continue;
        //     }
        //     co_yield static_cast<size_type>(
        //         difference_type(N) * (p - _data.get()) +
        //         static_cast<difference_type>(index));
        //     ++index;
        // }

        const span_type * data = _data.get();
        const size_type last_out_index = _size / N;
        const size_type last_in_index = _size & span_index_mask;

        struct {
            size_type out_index;
            size_type in_index;
        } cursor(0, 0);
        for(;;) {
            cursor.in_index += static_cast<size_type>(
                std::countr_zero((data[cursor.out_index]) >> cursor.in_index));
            if(cursor.out_index == last_out_index &&
               cursor.in_index >= last_in_index)
                break;
            if(cursor.in_index >= N) {
                ++cursor.out_index;
                cursor.in_index = 0;
                continue;
            }
            break;
        }

        return intrusive_view(
            cursor,
            [](const auto & cur) -> size_type {
                return cur.out_index * size_type(N) + cur.in_index;
            },
            [ data, last_out_index, last_in_index ](auto cur) -> auto{
                ++cur.in_index;
                for(;;) {
                    cur.in_index += static_cast<size_type>(std::countr_zero(
                        (data[cur.out_index]) >> cur.in_index));
                    if(cur.out_index == last_out_index &&
                       cur.in_index >= last_in_index)
                        return cur;
                    if(cur.in_index >= N) {
                        ++cur.out_index;
                        cur.in_index = 0;
                        continue;
                    }
                    return cur;
                }
            },
            [last_out_index, last_in_index](const arc a) -> bool {
                return cur.out_index != last_out_index ||
                       cur.in_index < last_in_index;
            });
    }
};

}  // namespace melon
}  // namespace fhamonic

#endif  // MELON_STATIC_MAP_BOOL_HPP