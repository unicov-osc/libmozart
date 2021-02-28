/**
 * Mozart++ Template Library: Core Library/Type Traits
 * Licensed under Apache 2.0
 * Copyright (C) 2020-2021 Chengdu Covariant Technologies Co., LTD.
 * Website: https://covariant.cn/
 * Github:  https://github.com/chengdu-zhirui/
 */

#pragma once

#include <type_traits>

#define mpp_in_place_type_t(T)  mpp_impl::in_place_t(&)(mpp_impl::in_place_type_tag<T>)
#define mpp_in_place_index_t(T)  mpp_impl::in_place_t(&)(mpp_impl::in_place_index_tag<I>)

namespace mpp_impl {
    template <typename T>
    struct in_place_type_tag {
    };

    template <std::size_t I>
    struct in_place_index_tag {
    };

    struct in_place_t {
    };

    template <typename T>
    inline in_place_t in_place(in_place_type_tag<T> = in_place_type_tag<T>()) {
        return in_place_t{};
    }

    template <std::size_t I>
    inline in_place_t in_place(in_place_index_tag<I> = in_place_index_tag<I>()) {
        return in_place_t{};
    }

    template <typename T>
    inline in_place_t in_place_type(in_place_type_tag<T> = in_place_type_tag<T>()) {
        return in_place_t{};
    }

    template <std::size_t I>
    inline in_place_t in_place_index(in_place_index_tag<I> = in_place_index_tag<I>()) {
        return in_place_t{};
    }
}

namespace mpp {
    struct true_type {
        constexpr static bool value = true;
    };

    struct false_type {
        constexpr static bool value = false;
    };

    /*
     * Something to simulate the requires keyword introduced in C++20.
     */

    template <typename ...>
    using requires_all = void;

    template <bool>
    struct requires_true_ {
    };

    template <>
    struct requires_true_<true> {
        using type = void;
    };

    template <bool b>
    using requires_true = typename requires_true_<b>::type;

    template <typename T, typename R>
    using requires_same = requires_true<std::is_same<T, R>::value>;

    template <typename T>
    using remove_cr_t = std::remove_const_t<std::remove_reference_t<T>>;

    /**
     * Integer Sequence
     * usage: using seq = make_sequence_t<Maximum>;
     * @see mpp::make_sequence_t
     */
    template <unsigned int... seq>
    struct sequence final {
    };

    template <unsigned int N, unsigned int... seq>
    struct make_sequence : public make_sequence<N - 1, N - 1, seq...> {
    };

    template <unsigned int... seq>
    struct make_sequence<0, seq...> {
        using type = sequence<seq...>;
    };

    /**
     * Short for {@code typename make_sequence<Maximum>::type}
     */
    template <unsigned int N>
    using make_sequence_t = typename make_sequence<N>::type;

    struct typelist {
    private:
        template <typename Seq, typename T>
        struct cons_impl;

        template <typename Seq>
        struct head_impl;

        template <typename Seq>
        struct tail_impl;

        template <typename SeqL, typename SeqR>
        struct concat_impl;

        template <typename Seq>
        struct size_impl;

        template <typename T, template <typename...> class Seq, typename... Ts>
        struct cons_impl<T, Seq<Ts...>> {
            using type = Seq<T, Ts...>;
        };

        template <template <typename...> class Seq, typename T, typename... Ts>
        struct head_impl<Seq<T, Ts...>> {
            using type = T;
        };

        template <template <typename...> class Seq, typename T, typename... Ts>
        struct tail_impl<Seq<T, Ts...>> {
            using type = Seq<Ts...>;
        };

        template <template <typename...> class Seq, typename... Ts, typename ... Us>
        struct concat_impl<Seq<Ts...>, Seq<Us ...>> {
            using type = Seq<Ts..., Us...>;
        };

        template <template <typename...> class Seq, typename... Ts>
        struct size_impl<Seq<Ts...>> {
            using type = std::integral_constant<std::size_t, sizeof...(Ts)>;
        };

        template <typename Seq, size_t Index>
        struct visit_impl {
            using type = typename visit_impl<typename tail_impl<Seq>::type, Index - 1>::type;
        };

        template <typename Seq>
        struct visit_impl<Seq, 0> {
            using type = typename head_impl<Seq>::type;
        };

    public:
        template <typename ...>
        struct list {
        };

        using nil = list<>;

        template <typename T, typename Seq>
        using cons = typename cons_impl<T, Seq>::type;

        template <typename Seq>
        using head = typename head_impl<Seq>::type;

        template <typename Seq>
        using tail = typename tail_impl<Seq>::type;

        template <typename Seq, size_t Index>
        using visit = typename visit_impl<Seq, Index>::type;

        template <typename SeqL, typename SeqR>
        using concat = typename concat_impl<SeqL, SeqR>::type;

        template <typename Seq>
        using size = typename size_impl<Seq>::type;

        template <typename Seq>
        using empty = std::is_same<size<Seq>, std::integral_constant<size_t, 0>>;

        template <typename SeqL, typename SeqR>
        using equals = std::is_same<SeqL, SeqR>;

        template <typename Seq>
        static constexpr size_t size_v = size<Seq>::value;

        template <typename Seq>
        static constexpr bool empty_v = empty<Seq>::value;

        template <typename SeqL, typename SeqR>
        static constexpr bool equals_v = equals<SeqL, SeqR>::value;
    };

    template <typename, typename = mpp::requires_all<>>
    struct is_iterable : public mpp::false_type {};

    template <typename CharT>
    struct is_iterable<std::basic_string<CharT>> : public mpp::false_type {};

    template <typename T>
    struct is_iterable<T,
        mpp::requires_all<
            typename T::iterator,
            typename T::const_iterator,
            requires_same<typename T::iterator, decltype(std::declval<T>().begin())>,
            requires_same<typename T::iterator, decltype(std::declval<T>().end())>,
            requires_same<typename T::const_iterator, decltype(std::declval<T>().cbegin())>,
            requires_same<typename T::const_iterator, decltype(std::declval<T>().cend())>
        >> : public mpp::true_type {
    };

    template <typename T>
    static constexpr bool is_iterable_v = is_iterable<mpp::remove_cr_t<T>>::value;

    template <typename, typename = void>
    struct iterable_traits {};

    template <typename Container>
    struct iterable_traits<Container, std::enable_if_t<is_iterable_v<Container>>> {
        using iterator_type = typename mpp::remove_cr_t<Container>::iterator;
        using const_iterator_type = typename mpp::remove_cr_t<Container>::const_iterator;

        static constexpr iterator_type begin(Container &&c) {
            return c.begin();
        }

        static constexpr iterator_type end(Container &&c) {
            return c.end();
        }

        static constexpr const_iterator_type cbegin(Container &&c) {
            return c.cbegin();
        }

        static constexpr const_iterator_type cend(Container &&c) {
            return c.cbegin();
        }
    };
}
