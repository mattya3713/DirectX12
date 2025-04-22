#pragma once

#include <utility>
#include <cassert>
#include <tuple>
#include <cstddef> // nullptr_t.

namespace MyPair {

    template<typename T1, typename T2>
    class pair {
    protected:
        std::pair<T1, T2> m_pr;

    public:
        constexpr pair() = default;
        constexpr pair(T1 a, T2 b) : m_pr(a, b) {}
        constexpr explicit pair(std::pair<T1, T2> pr_) : m_pr(std::move(pr_)) {}

        // [0] ‚ðˆÓ–¡‚·‚é“ÁŽêŒ^.
        constexpr T1& operator[](std::nullptr_t) {
            return m_pr.first;
        }

        constexpr const T1& operator[](std::nullptr_t) const {
            return m_pr.first;
        }

        // [1] ‚ðˆÓ–¡‚·‚é“ÁŽêƒ^ƒOŒ^.
        struct second_tag {
            constexpr explicit second_tag(int x) {
                assert(x == 1);
            }
        };

        constexpr T2& operator[](second_tag) {
            return m_pr.second;
        }

        constexpr const T2& operator[](second_tag) const {
            return m_pr.second;
        }

        // ”äŠr‰‰ŽZŽq.
        friend constexpr bool operator==(const pair& a, const pair& b) {
            return a.m_pr == b.m_pr;
        }

        friend constexpr bool operator!=(const pair& a, const pair& b) {
            return !(a == b);
        }

        // –¾Ž¦“I‚È•ÏŠ·.
        constexpr explicit operator std::pair<T1, T2>() const {
            return m_pr;
        }

        // std::get ŒÝŠ·.
        template<std::size_t I>
        constexpr decltype(auto) get() {
            if constexpr (I == 0) return (m_pr.first);
            else static_assert(I == 1, "Index out of bounds for pair");
            return (m_pr.second);
        }

        template<std::size_t I>
        constexpr decltype(auto) get() const {
            if constexpr (I == 0) return (m_pr.first);
            else static_assert(I == 1, "Index out of bounds for pair");
            return (m_pr.second);
        }
    };

} // namespace MyPair.

namespace std {
    template<std::size_t I, typename T1, typename T2>
    struct tuple_element<I, MyPair::pair<T1, T2>> {
        using type = std::conditional_t<I == 0, T1, T2>;
    };

    template<typename T1, typename T2>
    struct tuple_size<MyPair::pair<T1, T2>> : std::integral_constant<std::size_t, 2> {};

    template<std::size_t I, typename T1, typename T2>
    constexpr auto& get(MyPair::pair<T1, T2>& p) {
        return p.template get<I>();
    }

    template<std::size_t I, typename T1, typename T2>
    constexpr const auto& get(const MyPair::pair<T1, T2>& p) {
        return p.template get<I>();
    }
}
