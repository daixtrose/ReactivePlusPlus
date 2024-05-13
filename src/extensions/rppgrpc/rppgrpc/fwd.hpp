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

#include <rpp/observables/fwd.hpp>

#include <rpp/utils/constraints.hpp>

/**
 * @defgroup rppgrpc RPPGRPC
 * @brief RppGrpc is extension of RPP which enables support of grpc library.
 */

namespace grpc
{
    class ClientContext;

    template<class Request, class Response>
    class ClientBidiReactor;

    template<class Response>
    class ClientReadReactor;
} // namespace grpc

namespace rppgrpc
{
    template<typename Async, rpp::constraint::observable Observable, rpp::constraint::observer Observer>
    using member_bidi_function_ptr = void (Async::*)(grpc::ClientContext*, grpc::ClientBidiReactor<rpp::utils::extract_observable_type_t<Observable>, rpp::utils::extract_observer_type_t<Observer>>*);

    template<typename Async, typename Input, rpp::constraint::observer Observer>
    using member_read_function_ptr = void (Async::*)(grpc::ClientContext*, const Input*, grpc::ClientReadReactor<rpp::utils::extract_observer_type_t<Observer>>*);

    template<typename AsyncInMethod,
             std::derived_from<AsyncInMethod> Async,
             rpp::constraint::observable      Observable,
             rpp::constraint::observer        Observer>
    void add_reactor(grpc::ClientContext*                                          context,
                     Async&                                                        async,
                     member_bidi_function_ptr<AsyncInMethod, Observable, Observer> method,
                     const Observable&                                             inputs,
                     Observer&&                                                    outputs);

    template<typename AsyncInMethod,
             std::derived_from<AsyncInMethod> Async,
             typename Input,
             rpp::constraint::observer Observer>
    void add_reactor(grpc::ClientContext*                                     context,
                     Async&                                                   async,
                     const Input*                                             input,
                     member_read_function_ptr<AsyncInMethod, Input, Observer> method,
                     Observer&&                                               outputs);
} // namespace rppgrpc
