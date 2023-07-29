//                  ReactivePlusPlus library
//
//          Copyright Aleksey Loginov 2023 - present.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          https://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/victimsnino/ReactivePlusPlus
//

#pragma once

#include <rpp/operators/fwd.hpp>
#include <rpp/defs.hpp>
#include <rpp/operators/details/strategy.hpp>

#include <type_traits>

namespace rpp::operators::details
{
template<rpp::constraint::decayed_type Type, rpp::constraint::decayed_type EqualityFn>
struct distinct_until_changed_observer_strategy
{
    using DisposableStrategy = rpp::details::none_disposable_strategy;

    RPP_NO_UNIQUE_ADDRESS EqualityFn comparator;
    mutable std::optional<Type>      last_value{};

    template<typename T>
    void on_next(const rpp::constraint::observer auto& obs, T&& v) const
    {
        if (last_value.has_value() && comparator(utils::as_const(last_value.value()), rpp::utils::as_const(v)))
            return;

        last_value.emplace(std::forward<T>(v));
        obs.on_next(utils::as_const(last_value.value()));
    }

    constexpr static forwarding_on_error_strategy on_error{};
    constexpr static forwarding_on_completed_strategy on_completed{};
    constexpr static forwarding_set_upstream_strategy set_upstream{};
    constexpr static forwarding_is_disposed_strategy is_disposed{};
    constexpr static empty_on_subscribe on_subscribe{};
};

template<rpp::constraint::decayed_type EqualityFn>
struct distinct_until_changed_t : public operators::details::template_operator_observable_strategy<distinct_until_changed_observer_strategy, EqualityFn>
{
    template<rpp::constraint::decayed_type T>
        requires rpp::constraint::invocable_r_v<bool, EqualityFn, T, T>
    using ResultValue = T;
};
}

namespace rpp::operators
{
/**
 * @brief Suppress consecutive duplicates of emissions from original observable
 *
 * @marble distinct_until_changed
 {
     source observable       : +--1-1-2-2-3-2-1-|
     operator "distinct_until_changed" : +--1---2---3-2-1-|
 }
 *
 * @details Actually this operator has `std::optional` with last item and checks everytime where new emission is same or not.
 *
 * @par Performance notes:
 * - No any heap allocations at all
 * - std::optional to keep last value
 * - passing last and emitted value to predicate
 *
 * @param equality_fn optional equality comparator function
 * @warning #include <rpp/operators/distinct_until_changed.hpp>
 *
 * @par Example
 * @snippet distinct_until_changed.cpp distinct_until_changed
 * @snippet distinct_until_changed.cpp distinct_until_changed_with_comparator
 *
 * @ingroup filtering_operators
 * @see https://reactivex.io/documentation/operators/distinct.html
 */
template<typename EqualityFn>
    requires (!utils::is_not_template_callable<EqualityFn> || std::same_as<bool, std::invoke_result_t<EqualityFn, utils::convertible_to_any, utils::convertible_to_any>>)
auto distinct_until_changed(EqualityFn&& equality_fn)
{
    return details::distinct_until_changed_t<std::decay_t<EqualityFn>>{std::forward<EqualityFn>(equality_fn)};
}
} // namespace rpp::operators