#pragma once

#include <rpp/observables/constraints.hpp>
#include <rpp/operators/fwd/publish.hpp>
#include <rpp/operators/multicast.hpp>

IMPLEMENTATION_FILE(publish_tag);

namespace rpp::operators
{
template<typename ...Args>
auto publish() requires details::is_header_included<details::publish_tag, Args...>
{
    return []<constraint::observable TObservable>(TObservable&& observable)
    {
        return std::forward<TObservable>(observable).publish();
    };
}
} // namespace rpp::operators

namespace rpp::details
{
template<constraint::decayed_type Type, typename SpecificObservable>
template<constraint::decayed_same_as<SpecificObservable> TThis>
auto member_overload<Type, SpecificObservable, publish_tag>::publish_impl(TThis&& observable)
{
    return std::forward<TThis>(observable).multicast(rpp::subjects::publish_subject<Type>{});
}
} // namespace rpp::details
