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

#include <rpp/defs.hpp>
#include <rpp/observers/fwd.hpp>
#include <rpp/sources/fwd.hpp>
#include <rpp/disposables/disposable_wrapper.hpp>
#include <rpp/observables/observable.hpp>
#include <rpp/observables/details/chain_strategy.hpp>
#include <rpp/utils/constraints.hpp>
#include <rpp/utils/tuple.hpp>
#include <rpp/utils/utils.hpp>

#include <exception>
#include <variant>


namespace rpp::operators::details::constraint
{
template<typename S, typename Type>
concept operator_strategy = requires(const S& const_strategy,
                                     S& strategy,
                                     const Type& v,
                                     Type&& mv,
                                     const disposable_wrapper disposable,
                                     const rpp::details::fake_observer<Type>& const_observer,
                                     rpp::details::fake_observer<Type>& observer)
{
    const_strategy.on_subscribe(observer);

    const_strategy.on_next(const_observer, v);
    const_strategy.on_next(const_observer, std::move(mv));
    const_strategy.on_error(observer, std::exception_ptr{});
    const_strategy.on_completed(observer);

    strategy.set_upstream(observer, disposable);
    { strategy.is_disposed() } -> std::same_as<bool>;
};
}
namespace rpp::operators::details
{
template<rpp::constraint::decayed_type T, rpp::constraint::observer TObs, constraint::operator_strategy<rpp::utils::extract_observer_type_t<TObs>> Strategy>
class operator_strategy_base;

template<rpp::constraint::decayed_type T, rpp::constraint::decayed_type TT, rpp::constraint::observer_strategy<TT> ObserverStrategy, constraint::operator_strategy<TT> Strategy>
class operator_strategy_base<T, rpp::observer<TT, ObserverStrategy>, Strategy>
{
public:
    using DisposableStrategy = rpp::details::deduce_disposable_strategy_t<Strategy>;

    /**
     * @brief Construct a new operator strategy for passed observer
     * @warning Passed observer would not be moved inside, only rvalue reference saved inside. Actual move happens ONLY in case of move of this strategy
     */
    template<rpp::constraint::decayed_same_as<rpp::observer<TT, ObserverStrategy>> TObserver, typename...Args>
    operator_strategy_base(TObserver&& observer, Args&&...args)
        : m_observer{std::forward<TObserver>(observer)}
        , m_strategy{std::forward<Args>(args)...}
        {
            m_strategy.on_subscribe(m_observer);
        }

    operator_strategy_base(const operator_strategy_base&) = delete;
    operator_strategy_base(operator_strategy_base&& other) noexcept = default;

    operator_strategy_base& operator=(const operator_strategy_base&) = delete;
    operator_strategy_base& operator=(operator_strategy_base&&) = delete;

    ~operator_strategy_base() noexcept = default;

    void set_upstream(const disposable_wrapper& d)     { m_strategy.set_upstream(m_observer, d); }
    bool is_disposed() const                           { return m_strategy.is_disposed() || m_observer.is_disposed(); }

    void on_next(const T& v) const                     { m_strategy.on_next(m_observer, v); }
    void on_next(T&& v) const                          { m_strategy.on_next(m_observer, std::move(v)); }
    void on_error(const std::exception_ptr& err) const { m_strategy.on_error(m_observer, err); }
    void on_completed() const                          { m_strategy.on_completed(m_observer); }

private:
    mutable rpp::observer<TT, ObserverStrategy> m_observer;
    RPP_NO_UNIQUE_ADDRESS Strategy              m_strategy;
};

struct forwarding_on_next_strategy
{
    template<typename T>
    void operator()(const rpp::constraint::observer auto& obs, T&& v) const
    {
        obs.on_next(std::forward<T>(v));
    }
};

struct forwarding_on_error_strategy
{
    void operator()(const rpp::constraint::observer auto & obs, const std::exception_ptr& err) const
    {
        obs.on_error(err);
    }
};

struct forwarding_on_completed_strategy
{
    void operator()(const rpp::constraint::observer auto& obs) const
    {
        obs.on_completed();
    }
};

struct forwarding_set_upstream_strategy
{
    template<rpp::constraint::decayed_type T, rpp::constraint::observer_strategy<T> ObserverStrategy>
    void operator()(rpp::observer<T, ObserverStrategy>& observer, const rpp::disposable_wrapper& d) const {observer.set_upstream(d); }
};

struct forwarding_is_disposed_strategy
{
    bool operator()() const {return false; }
};

struct empty_on_subscribe
{
    void operator()(const rpp::constraint::observer auto&) const {}
};

template<typename SubscribeStrategy, rpp::constraint::decayed_type... Args>
class operator_observable_strategy_base
{
public:
    template<rpp::constraint::decayed_same_as<Args> ...TArgs>
    operator_observable_strategy_base(TArgs&&...args)
        : m_vals{std::forward<TArgs>(args)...} {}

    template<rpp::constraint::observer Observer, typename... Strategies>
    void subscribe(Observer&& observer, const observable_chain_strategy<Strategies...>& strategy) const
    {
        m_vals.apply(&SubscribeStrategy::template apply<Observer, observable_chain_strategy<Strategies...>, Args...>, std::forward<Observer>(observer), strategy);
    }

private:
    RPP_NO_UNIQUE_ADDRESS rpp::utils::tuple<Args...> m_vals{};
};

template<template<typename...> typename Strategy>
struct identity_subscribe_strategy
{
    template<rpp::constraint::observer Observer, typename ObservableStrategy, typename ...Args>
    static void apply(Observer&& observer, const ObservableStrategy& strategy, const Args&... vals)
    {
        using Type = typename ObservableStrategy::ValueType;
        strategy.subscribe(rpp::observer<Type, operator_strategy_base<Type, std::decay_t<Observer>, Strategy<Args...>>>{std::forward<Observer>(observer), vals...});
    }
};

template<template<typename...> typename Strategy, rpp::constraint::decayed_type... Args>
using operator_observable_strategy = operator_observable_strategy_base<identity_subscribe_strategy<Strategy>, Args...>;

template<template<typename, typename...> typename Strategy>
struct template_subscribe_strategy
{
    template<rpp::constraint::observer Observer, typename ObservableStrategy, typename ...Args>
    static void apply(Observer&& observer, const ObservableStrategy& strategy, const Args&... vals)
    {
        using Type = typename ObservableStrategy::ValueType;
        strategy.subscribe(rpp::observer<Type, operator_strategy_base<Type, std::decay_t<Observer>, Strategy<Type, Args...>>>{std::forward<Observer>(observer), vals...});
    }
};

template<template<typename, typename...> typename Strategy, rpp::constraint::decayed_type... Args>
using template_operator_observable_strategy = operator_observable_strategy_base<template_subscribe_strategy<Strategy>, Args...>;

template<typename Strategy>
struct not_template_subscribe_strategy
{
    template<rpp::constraint::observer Observer, typename ObservableStrategy, typename ...Args>
    static void apply(Observer&& observer, const ObservableStrategy& strategy, const Args&... vals)
    {
        using Type = typename ObservableStrategy::ValueType;
        strategy.subscribe(rpp::observer<Type, operator_strategy_base<Type, std::decay_t<Observer>, Strategy>>{std::forward<Observer>(observer), vals...});
    }
};
template<typename Strategy, rpp::constraint::decayed_type... Args>
using not_template_operator_observable_strategy = operator_observable_strategy_base<not_template_subscribe_strategy<Strategy>, Args...>;
}