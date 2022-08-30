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
// Created on: Sep 6, 2018
//     Author: oelsnerc
//******************************************************************************
#include "asynchronous/waiter.hpp"

#include <thread>
#include <future>
#include <vector>
#include <atomic>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//******************************************************************************
template<typename... ARGS>
inline std::future<void> run(ARGS&&... args)
{ return std::async(std::launch::async, std::forward<ARGS>(args)...); }

//******************************************************************************
TEST(Test_Waiter, simple)
{
    constexpr int count = 42;
    auto w = asynchronous::createWaiter(0, asynchronous::isGreaterThan(count));

    std::thread t([&]()
            {
                for(int i = 0; i < 2*count; ++i)
                { w+=1; }
            });

    w.wait();
    EXPECT_LE(count, w.getValue()); // the value was reached once, but might have changed already

    t.join();
    EXPECT_EQ(2*count, w.getValue());
}

TEST(Test_Waiter, latch)
{
    auto expected_text = "done";
    auto w = asynchronous::createWaiter(1, asynchronous::isEqualTo(0));
    std::string text;

    auto f = run([&]()
        {
             text = expected_text;
             w-=1;
        });

    w.wait();

    EXPECT_EQ(0, w.getValue());
    EXPECT_EQ(expected_text, text);
}

TEST(Test_Waiter, barrier)
{
    constexpr int count = 4;

    auto w = asynchronous::createWaiter(count, asynchronous::isEqualTo(0));
    std::atomic_int sum{0};

    std::vector<std::future<void>> threads;
    for(int i = 1; i < count; ++i)
    {
        threads.emplace_back(run([&,i]()
                {
                    sum+=i;
                    w.modify_and_wait([](int& v) { --v; });
                }));
    }

    w.modify_and_wait([](int& v) { --v; });

    //TODO reset barrier to start value

    EXPECT_EQ(0, w.getValue());
    EXPECT_EQ(count*(count-1)/2, sum);
}

//------------------------------------------------------------------------------
struct HasSize
{
    const size_t value;

    bool test (const std::vector<int>& data) const
    { return ( value <= data.size() ); }

    bool setup (const std::vector<int>& data) const
    { return test(data); }
};

TEST(Test_Waiter, updater)
{
    constexpr size_t count = 4;

    asynchronous::Waiter<std::vector<int>, HasSize> w(HasSize{count});

    std::vector<std::future<void>> threads;
    for(size_t i = 0; i < count; ++i)
    {
        threads.emplace_back(run([&,i]()
                {
                    auto updater = w.getUpdater();
                    updater->emplace_back(i);
                }));
    }

    w.wait();

    EXPECT_EQ(count, w->size());
}


TEST( Test_Waiter, timeout )
{
    asynchronous::WaiterForZero<size_t> waiterForZero{1};

    EXPECT_FALSE( waiterForZero.wait_for(std::chrono::microseconds{1}) );

    waiterForZero.count_down();
    EXPECT_TRUE( waiterForZero.wait_for(std::chrono::microseconds{1}) );
}   // TEST timeout

//******************************************************************************
// EOF
//******************************************************************************
