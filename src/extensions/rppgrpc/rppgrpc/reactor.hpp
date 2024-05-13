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

#include <rpp/subjects/publish_subject.hpp>

#include <grpcpp/support/client_callback.h>
#include <rppgrpc/fwd.hpp>
#include <rppgrpc/utils/exceptions.hpp>

#include <deque>

namespace rppgrpc::details
{
    template<rpp::constraint::decayed_type Input, rpp::constraint::observer Observer>
    class bidi_reactor final : public grpc::ClientBidiReactor<Input, rpp::utils::extract_observer_type_t<Observer>>
    {
        using Output = rpp::utils::extract_observer_type_t<Observer>;
        using Base   = grpc::ClientBidiReactor<Input, Output>;

    public:
        template<rpp::constraint::observable_of_type<Input> Observable, rpp::constraint::decayed_same_as<Observer> TObserver>
        bidi_reactor(const Observable& messages, TObserver&& events)
            : m_observer{std::forward<TObserver>(events)}
            , m_disposable{messages.subscribe_with_disposable([this]<rpp::constraint::decayed_same_as<Input> T>(T&& message) {
                std::lock_guard lock{m_write_mutex};
                m_write.push_back(std::forward<T>(message));
                if (m_write.size() == 1)
                    Base::StartWrite(&m_write.front()); },
                                                              [this](const std::exception_ptr& err) {
                                                                  Base::StartWritesDone();
                                                              },
                                                              [this]() {
                                                                  Base::StartWritesDone();
                                                              })}
        {
        }

        void Init()
        {
            Base::StartCall();
            Base::StartRead(&m_read);
        }

    private:
        void OnReadDone(bool ok) override
        {
            if (!ok)
            {
                m_observer.on_error(std::make_exception_ptr(rppgrpc::utils::on_read_done_not_ok{"OnReadDone is not ok"}));
                Destroy();
                return;
            }
            m_observer.on_next(m_read);
            Base::StartRead(&m_read);
        }

        void OnWriteDone(bool ok) override
        {
            if (!ok)
            {
                m_observer.on_error(std::make_exception_ptr(rppgrpc::utils::on_write_done_not_ok{"OnWriteDone is not ok"}));
                Destroy();
                return;
            }

            std::lock_guard lock{m_write_mutex};
            m_write.pop_front();

            if (!m_write.empty())
            {
                Base::StartWrite(&m_write.front());
            }
        }

        void OnDone(const grpc::Status&) override
        {
            m_observer.on_completed();
            Destroy();
        }

    private:
        void Destroy()
        {
            m_disposable.dispose();
            delete this;
        }

    private:
        Observer                m_observer;
        rpp::disposable_wrapper m_disposable;

        Output m_read{};

        std::mutex        m_write_mutex{};
        std::deque<Input> m_write{};
    };

    template<rpp::constraint::observer Observer>
    class read_reactor final : public grpc::ClientReadReactor<rpp::utils::extract_observer_type_t<Observer>>
    {
        using Output = rpp::utils::extract_observer_type_t<Observer>;
        using Base   = grpc::ClientReadReactor<Output>;

    public:
        template<rpp::constraint::decayed_same_as<Observer> TObserver>
            requires (!rpp::constraint::decayed_same_as<TObserver, read_reactor<Observer>>)
        explicit read_reactor(TObserver&& events)
            : m_observer{std::forward<TObserver>(events)}
        {
        }

        read_reactor(read_reactor&&) = delete;

        void Init()
        {
            Base::StartCall();
            Base::StartRead(&m_read);
        }

    private:
        void OnReadDone(bool ok) override
        {
            if (!ok)
            {
                m_observer.on_error(std::make_exception_ptr(rppgrpc::utils::on_read_done_not_ok{"OnReadDone is not ok"}));
                Destroy();
                return;
            }
            m_observer.on_next(m_read);
            Base::StartRead(&m_read);
        }

        void OnDone(const grpc::Status&) override
        {
            m_observer.on_completed();
            Destroy();
        }

    private:
        void Destroy()
        {
            delete this;
        }

    private:
        Observer m_observer;
        Output   m_read{};
    };
} // namespace rppgrpc::details
namespace rppgrpc
{
    template<typename AsyncInMethod,
             std::derived_from<AsyncInMethod> Async,
             rpp::constraint::observable      Observable,
             rpp::constraint::observer        Observer>
    void add_reactor(grpc::ClientContext*                                          context,
                     Async&                                                        async,
                     member_bidi_function_ptr<AsyncInMethod, Observable, Observer> method,
                     const Observable&                                             inputs,
                     Observer&&                                                    outputs)
    {
        const auto reactor = new details::bidi_reactor<rpp::utils::extract_observable_type_t<Observable>, std::decay_t<Observer>>(inputs, std::forward<Observer>(outputs));
        (async.*method)(context, reactor);
        reactor->Init();
    }


    template<typename AsyncInMethod,
             std::derived_from<AsyncInMethod> Async,
             typename Input,
             rpp::constraint::observer Observer>
    void add_reactor(grpc::ClientContext*                                     context,
                     Async&                                                   async,
                     const Input*                                             input,
                     member_read_function_ptr<AsyncInMethod, Input, Observer> method,
                     Observer&&                                               outputs)
    {
        const auto reactor = new details::read_reactor<std::decay_t<Observer>>(std::forward<Observer>(outputs));
        (async.*method)(context, input, reactor);
        reactor->Init();
    }
} // namespace rppgrpc
