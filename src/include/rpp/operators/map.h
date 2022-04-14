// MIT License
// 
// Copyright (c) 2022 Aleksey Loginov
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <rpp/observables/constraints.h>
#include <rpp/subscribers/constraints.h>
#include <rpp/observables/type_traits.h>
#include <rpp/operators/fwd/map.h>
#include <rpp/utils/utilities.h>
#include <utility>

IMPLEMENTATION_FILE(map_tag);

namespace rpp::operators
{
template<typename Callable>
auto map(Callable&& callable) requires details::is_header_included<details::map_tag, Callable>
{
    return [callable = std::forward<Callable>(callable)]<constraint::observable TObservable>(TObservable&& observable)
    {
        return observable.map(callable);
    };
}
} // namespace rpp::operators

namespace rpp::details
{
template<constraint::decayed_type Type, typename SpecificObservable>
template<std::invocable<Type> Callable>
auto member_overload<Type, SpecificObservable, map_tag>::map_impl(Callable&& callable)
{
    return [callable = std::forward<Callable>(callable)](auto&& value, const constraint::subscriber auto& subscriber)
    {
        subscriber.on_next(callable(utilities::as_const(std::forward<decltype(value)>(value))));
    };
}
} // namespace rpp::details