/**
 * Mozart++ Template Library
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */


#pragma once

#include <mozart++/core>
#include <utility>

namespace mpp {
    /**
     * A range adaptor for a pair of iterators.
     * This just wraps two iterators into a range-compatible interface.
     *
     * @tparam IterT
     */
    template <typename IterT>
    class iterator_range {
        IterT _begin_iterator;
        IterT _end_iterator;

    public:
        iterator_range(IterT begin_iterator, IterT end_iterator)
            : _begin_iterator(std::move(begin_iterator)),
              _end_iterator(std::move(end_iterator)) {}

        IterT begin() const {
            return _begin_iterator;
        }

        IterT end() const {
            return _end_iterator;
        }

        bool empty() const {
            return _begin_iterator == _end_iterator;
        }
    };

    /*
     * Convenience function for iterating over sub-ranges.
     * This provides a bit of syntactic sugar to make using sub-ranges
     * in for loops a bit easier. Analogous to std::make_pair().
     */

    template <class T>
    iterator_range<T> make_range(T x, T y) {
        return iterator_range<T>(std::move(x), std::move(y));
    }

    template <typename Container>
    auto make_range(Container &&c) {
        static_assert(is_iterable_v<Container>, "not an iterable");
        using IterT = typename iterable_traits<Container>::iterator_type;
        return make_range<IterT>(iterable_traits<Container>::begin(std::forward<Container>(c)),
            iterable_traits<Container>::end(std::forward<Container>(c)));
    }
}
