//                   ReactivePlusPlus library
//
//           Copyright Aleksey Loginov 2022 - present.
//                     TC Wang 2022 - present.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           https://www.boost.org/LICENSE_1_0.txt)
//
//  Project home: https://github.com/victimsnino/ReactivePlusPlus

#include "mock_observer.hpp"
#include <test_scheduler.hpp>

#include <catch2/catch_test_macros.hpp>
#include <rpp/operators/delay.hpp>
#include <rpp/schedulers/trampoline_scheduler.hpp>
#include <rpp/subjects/publish_subject.hpp>
#include <rpp/sources/empty.hpp>
#include <rpp/sources/error.hpp>
#include <rpp/sources/just.hpp>

SCENARIO("delay mirrors both source observable and trigger observable", "[delay]")
{
    auto mock = mock_observer<int>{};
    std::chrono::milliseconds delay_duration{300};

    GIVEN("observable of -1-|")
    {
        const auto now  = rpp::schedulers::clock_type::now();

        rpp::source::just(1)
                .delay(delay_duration, rpp::schedulers::trampoline{})
                .as_blocking()
                .subscribe(
                           [&](auto&& v)
                           {
                               THEN("should see event after the delay")
                               {
                                   CHECK(rpp::schedulers::clock_type::now() >= now + delay_duration);
                               }

                               mock.on_next(v);
                           },
                           [&](const std::exception_ptr& err) { mock.on_error(err); },
                           [&]()
                           {
                               THEN("should see event after the delay")
                               {
                                   CHECK(rpp::schedulers::clock_type::now() >= now + delay_duration);
                               }

                               mock.on_completed();
                           });

        THEN("should see -1-|")
        {
            CHECK(mock.get_received_values() == std::vector<int>{1});
            CHECK(mock.get_on_completed_count() == 1);
            CHECK(mock.get_on_error_count() == 0);
        }
    }

    GIVEN("observable of -x")
    {
        const auto now  = rpp::schedulers::clock_type::now();

        rpp::source::error<int>(std::make_exception_ptr(std::runtime_error{""}))
                .delay(delay_duration, rpp::schedulers::trampoline{})
                .as_blocking()
                .subscribe([&](auto&&                    v) { mock.on_next(v); },
                           [&](const std::exception_ptr& err)
                           {
                               THEN("should see event immediately")
                               {
                                   CHECK(rpp::schedulers::clock_type::now() < now + delay_duration);
                               }
                               mock.on_error(err);
                           },
                           [&]() { mock.on_completed(); });

        THEN("should see -x after the delay")
        {
            CHECK(mock.get_received_values().empty());
            CHECK(mock.get_on_completed_count() == 0);
            CHECK(mock.get_on_error_count() == 1);
        }
    }

    GIVEN("observable of -|")
    {
        const auto now  = rpp::schedulers::clock_type::now();

        rpp::source::empty<int>()
                .delay(delay_duration, rpp::schedulers::trampoline{})
                .as_blocking()
                .subscribe([&](auto&&                    v) { mock.on_next(v); },
                           [&](const std::exception_ptr& err) { mock.on_error(err); },
                           [&]()
                           {
                               THEN("should see event after delay")
                               {
                                   CHECK(rpp::schedulers::clock_type::now() >= now + delay_duration);
                               }
                               mock.on_completed();
                           });

        THEN("should see -|")
        {
            CHECK(mock.get_received_values().empty());
            CHECK(mock.get_on_completed_count() == 1);
            CHECK(mock.get_on_error_count() == 0);
        }
    }

    GIVEN("subject with items")
    {
        auto subj = rpp::subjects::publish_subject<int>{};

        WHEN("subscribe on subject via delay and doing recursive submit from another thread")
        {
            THEN("all values obtained in the same thread")
            {
                auto current_thread = std::this_thread::get_id();

                auto sub = subj.get_observable()
                    .delay(delay_duration, rpp::schedulers::trampoline{})
                    .subscribe([&](int v)
                    {
                        CHECK(std::this_thread::get_id() == current_thread);

                        mock.on_next(v);

                        if (v == 1)
                        {
                            std::thread{[&]{subj.get_subscriber().on_next(2);}}.join();

                            THEN("no recursive on_next calls")
                            {
                                CHECK(mock.get_received_values() == std::vector{1});
                            }
                        }
                    });

                subj.get_subscriber().on_next(1);

                AND_THEN("all values obtained")
                {
                    CHECK(mock.get_received_values() == std::vector{ 1, 2 });
                }
            }
        }
        WHEN("subscribe on subject via delay via test_scheduler, sent value")
        {
            subj.get_observable()
                .delay(std::chrono::seconds{30000}, test_scheduler{})
                .subscribe(mock);

            subj.get_subscriber().on_next(1);

            AND_THEN("no memory leak")
            {
                // checked via sanitizer
            }
        }
    }
}