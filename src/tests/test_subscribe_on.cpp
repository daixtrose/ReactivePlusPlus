//                   ReactivePlusPlus library
// 
//           Copyright Aleksey Loginov 2022 - present.
//  Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at
//           https://www.boost.org/LICENSE_1_0.txt)
// 
//  Project home: https://github.com/victimsnino/ReactivePlusPlus

#include "mock_observer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <rpp/operators/subscribe_on.hpp>
#include <rpp/schedulers/new_thread_scheduler.hpp>

#include <optional>

TEST_CASE("subscribe_on schedules job in another thread")
{
    auto mock = mock_observer<int>{};
    GIVEN("observable")
    {
        std::optional<std::thread::id> thread_of_subscription{};
        auto                           obs = rpp::source::create<int>([&](const auto& sub)
        {
            thread_of_subscription = std::this_thread::get_id();
            sub.on_next(1);
            sub.on_completed();
        });
        WHEN("subscribe on it with subscribe_on")
        {
            obs.subscribe_on(rpp::schedulers::new_thread{}).as_blocking().subscribe(mock);
            THEN("expect to obtain value from another thread")
            {
                REQUIRE(mock.get_total_on_next_count() == 1);
                REQUIRE(mock.get_on_completed_count() == 1);
                CHECK(thread_of_subscription != std::this_thread::get_id());
            }
        }
    }
}