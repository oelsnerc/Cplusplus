/* begin copyright

   IBM Confidential

   Licensed Internal Code Source Materials

   3931, 3932 Licensed Internal Code

   (C) Copyright IBM Corp. 2019, 2022

   The source code for this program is not published or otherwise
   divested of its trade secrets, irrespective of what has
   been deposited with the U.S. Copyright Office.

   end copyright
*/

//******************************************************************************
// Created on: Sep 27, 2018
//     Author: oelsnerc
//******************************************************************************
#include "asynchronous/scheduler.hpp"
#include "asynchronous/latch.hpp"

#include <future>
#include <chrono>
#include <vector>
#include <iostream>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//------------------------------------------------------------------------------
using ms = std::chrono::milliseconds;
using Clock = asynchronous::Scheduler::Clock;

static void waitToStartThread(asynchronous::Scheduler& s)
{
    asynchronous::latch l(1);
    s.delay_for(ms(0), [&]() { l.count_down(); });
    l.wait();
}

//******************************************************************************
TEST(Test_Scheduler, empty)
{
    asynchronous::Scheduler s;

}

TEST(Test_Scheduler, simple)
{
    std::promise<int> promise;

    asynchronous::Scheduler s;

    s.delay_for(ms(1), [&promise]()
            {
                promise.set_value(42);
            });

    auto f = promise.get_future();

    EXPECT_EQ(42, f.get());
}

TEST(Test_Scheduler, order)
{
    asynchronous::Scheduler s;
    waitToStartThread(s);

    auto now = Clock::now();
    const auto tp_1 = now + ms(1);
    const auto tp_2 = now + ms(5);
    const auto tp_3 = now + ms(10);

    asynchronous::latch counter(3);
    std::vector<int> results;

    s.delay_until(tp_3, [&]() { results.emplace_back(3); counter.count_down(); });
    s.delay_until(tp_2, [&]() { results.emplace_back(2); counter.count_down(); });
    s.delay_until(tp_1, [&]() { results.emplace_back(1); counter.count_down(); });

    counter.wait();

    const std::vector<int> expected = { 1, 2, 3 };
    EXPECT_EQ(expected, results);
}

TEST(Test_Scheduler, self_scheduling)
{
    asynchronous::Scheduler s;
    waitToStartThread(s);

    constexpr auto max_count = 3;
    int number = 0;
    asynchronous::latch counter(max_count);
    std::function<void(void)> f = [&]()
        {
            ++number;
            if (counter.count_down()) { return; }
            s.delay_for(ms(5), f);
        };

    s.delay_for(ms(0), f);
    counter.wait();

    EXPECT_EQ(max_count, number);
}

//------------------------------------------------------------------------------
struct MyFunctor
{
    size_t times_called;
    asynchronous::latch counter;

    explicit MyFunctor(size_t max_count) :
            times_called(0),
            counter(max_count)
    {}

    void operator () ()
    {
        ++times_called;
        try
        {
            counter.count_down();   // throws if already zero
        } catch(...) {}
    }
};


TEST(Test_Scheduler, object)
{
    asynchronous::Scheduler s;
    waitToStartThread(s);

    constexpr auto max_count = 3u;

    MyFunctor func(max_count);

    s.delay_for(ms(1), std::ref(func));
    s.delay_for(ms(1), std::ref(func));
    s.delay_for(ms(1), std::ref(func));

    func.counter.wait();

    EXPECT_EQ(max_count, func.times_called);
}

//******************************************************************************
// EOF
//******************************************************************************
