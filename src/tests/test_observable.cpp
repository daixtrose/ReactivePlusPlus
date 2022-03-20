// MIT License
// 
// Copyright (c) 2021 Aleksey Loginov
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

#include "copy_count_tracker.h"

#include <catch2/catch_test_macros.hpp>

#include <rpp/observable.h>
#include <rpp/observer.h>
#include <rpp/subscriber.h>

#include <array>

SCENARIO("Any observable can be casted to dynamic_observable", "[observable]")
{
    auto validate_observable =[](const auto& observable)
    {
        WHEN("Call as_dynamic function")
        {
            auto dynamic_observable = observable.as_dynamic();

            THEN("Obtain dynamic_observable of same type")
            {
                static_assert(std::is_same<decltype(dynamic_observable), rpp::dynamic_observable<int>>{}, "Type of dynamic observable should be same!");
            }
        }

        WHEN("Construct dynamic_observable by constructor")
        {
            auto dynamic_observable = rpp::dynamic_observable{observable};

            THEN("Obtain dynamic_observable of same type")
            {
                static_assert(std::is_same<decltype(dynamic_observable), rpp::dynamic_observable<int>>{}, "Type of dynamic observable should be same!");
            }
        }
    };

    GIVEN("specific_observable")
    {
        validate_observable(rpp::specific_observable([](const rpp::dynamic_subscriber<int>&) {}));
    }

    GIVEN("dynamic_observable")
    {
        validate_observable(rpp::dynamic_observable([](const rpp::dynamic_subscriber<int>&) {}));
    }
}

template<typename ObserverGetValue, bool is_move = false, bool is_const = false>
static void TestObserverTypes(const std::string then_description, int copy_count, int move_count)
{
    GIVEN("observer and observable of same type")
    {
        std::conditional_t<is_const, const copy_count_tracker, copy_count_tracker> tracker{};
        const auto observer             = rpp::dynamic_observer{[](ObserverGetValue) {  }};

        const auto observable = rpp::observable::create([&](const rpp::dynamic_subscriber<copy_count_tracker>& sub)
        {
            if constexpr (is_move)
                sub.on_next(std::move(tracker));
            else
                sub.on_next(tracker);
        });

        WHEN("subscribe called for observble")
        {
            observable.subscribe(observer);

            THEN(then_description)
            {
                CHECK(tracker.get_copy_count() == copy_count);
                CHECK(tracker.get_move_count() == move_count);
            }
        }
    }
}

SCENARIO("specific_observable doesn't produce extra copies for lambda", "[observable][track_copy]")
{
    GIVEN("observer and specific_observable of same type")
    {
        copy_count_tracker tracker{};
        const auto observer             = rpp::dynamic_observer{[](int) {  }};

        const auto observable = rpp::observable::create([tracker](const rpp::dynamic_subscriber<int>& sub)
        {
            sub.on_next(123);
        });

        WHEN("subscribe called for observble")
        {
            observable.subscribe(observer);

            THEN("One copy of tracker into lambda, one move of lambda into internal state")
            {
                CHECK(tracker.get_copy_count() == 1);
                CHECK(tracker.get_move_count() == 1);
            }
            AND_WHEN("Make copy of observable")
            {
                auto copy_of_observable = observable;
                THEN("One more copy of lambda")
                {
                    CHECK(tracker.get_copy_count() == 2);
                    CHECK(tracker.get_move_count() == 1);
                }
            }
        }

    }
}

SCENARIO("dynamic_observable doesn't produce extra copies for lambda", "[observable][track_copy]")
{
    GIVEN("observer and dynamic_observable of same type")
    {
        copy_count_tracker tracker{};
        const auto observer             = rpp::dynamic_observer{[](int) {  }};

        const auto observable = rpp::observable::create([tracker](const rpp::dynamic_subscriber<int>& sub)
        {
            sub.on_next(123);
        }).as_dynamic();

        WHEN("subscribe called for observble")
        {
            observable.subscribe(observer);

            THEN("One copy of tracker into lambda, one move of lambda into internal state and one move to dynamic observable")
            {
                CHECK(tracker.get_copy_count() == 1);
                CHECK(tracker.get_move_count() == 2);
            }
            AND_WHEN("Make copy of observable")
            {
                auto copy_of_observable = observable;
                THEN("No any new copies of lambda")
                {
                    CHECK(tracker.get_copy_count() == 1);
                    CHECK(tracker.get_move_count() == 2);
                }
            }
        }

    }
}

SCENARIO("Verify copy when observer take lvalue from lvalue&", "[observable][track_copy]")
{
    TestObserverTypes<copy_count_tracker>("1 copy to final lambda", 1, 0);
}

SCENARIO("Verify copy when observer take lvalue from move", "[observable][track_copy]")
{
    TestObserverTypes<copy_count_tracker, true>("1 move to final lambda", 0, 1);
}

SCENARIO("Verify copy when observer take lvalue from const lvalue&", "[observable][track_copy]")
{
    TestObserverTypes<copy_count_tracker,false, true>("1 copy to final lambda", 1, 0);
}

SCENARIO("Verify copy when observer take const lvalue& from lvalue&", "[observable][track_copy]")
{
    TestObserverTypes<const copy_count_tracker&>("no copies", 0, 0);
}

SCENARIO("Verify copy when observer take const lvalue& from move", "[observable][track_copy]")
{
    TestObserverTypes<const copy_count_tracker&, true>("no copies", 0, 0);
}

SCENARIO("Verify copy when observer take const lvalue& from const lvalue&", "[observable][track_copy]")
{
    TestObserverTypes<const copy_count_tracker&,false, true>("no copies", 0, 0);
}