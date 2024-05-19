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

#include <grpcpp/support/server_callback.h>
#include <rppgrpc/fwd.hpp>
#include <rppgrpc/utils/exceptions.hpp>

#include <list>

namespace rppgrpc::details
{
    template<rpp::constraint::decayed_type Response, rpp::constraint::observer Observer>
    class server_bidi_reactor final : public grpc::ServerBidiReactor<rpp::utils::extract_observer_type_t<Observer>, Response>
    {
        using Request = rpp::utils::extract_observer_type_t<Observer>;
        using Base    = grpc::ServerBidiReactor<Request, Response>;

    public:
        template<rpp::constraint::observable_of_type<Response> Observable, rpp::constraint::decayed_same_as<Observer> TObserver>
        server_bidi_reactor(const Observable& messages, TObserver&& events)
            : m_observer{std::forward<TObserver>(events)}
            , m_disposable{messages.subscribe_with_disposable([this]<rpp::constraint::decayed_same_as<Response> T>(T&& message) {
                std::lock_guard lock{m_write_mutex};
                m_write.push_back(std::forward<T>(message));
                if (m_write.size() == 1)
                    Base::StartWrite(&m_write.front()); },
                                                              [this](const std::exception_ptr& err) {
                                                                  Base::Finish(grpc::Status{grpc::StatusCode::INTERNAL, ""});
                                                              },
                                                              [this]() {
                                                                  Base::Finish(grpc::Status::OK);
                                                              })}
        {
            Base::StartSendInitialMetadata();
            Base::StartRead(&m_read);
        }

    private:
        void OnReadDone(bool ok) override
        {
            if (!ok)
            {
                m_observer.on_error(std::make_exception_ptr(rppgrpc::utils::reactor_faield{"OnReadDone is not ok"}));
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
                m_observer.on_error(std::make_exception_ptr(rppgrpc::utils::reactor_faield{"OnWriteDone is not ok"}));
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

        void OnDone() override
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

        Request m_read{};

        std::mutex          m_write_mutex{};
        std::list<Response> m_write{};
    };
} // namespace rppgrpc::details
namespace rppgrpc
{
} // namespace rppgrpc
