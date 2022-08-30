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
// Created on: Jun 6, 2018
//     Author: oelsnerc
//******************************************************************************

#include <chrono>
#include <thread>
#include <exception>
#include <mutex>
#include <list>

#include "asynchronous/repeat.hpp"
#include "asynchronous/latch.hpp"

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//------------------------------------------------------------------------------
using ms = std::chrono::milliseconds;
using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

static void increase(unsigned int* a, asynchronous::latch* l)
{
    ++(*a);
    l->count_down();
}

//******************************************************************************
TEST(TestRepeat, basic)
{
    unsigned int counter = 0;
    constexpr unsigned int expected_count = 2;
    asynchronous::latch done(expected_count);

    {
        auto g = asynchronous::repeat(ms(5), increase, &counter, &done);
        done.wait();
    }

    EXPECT_LE(expected_count, counter);
}

// Compile Error (as expected):
// Test_Repeat.cpp:58:5: error: ignoring return value of function declared with 'warn_unused_result' attribute [-Werror,-Wunused-result]
//     asynchronous::repeat(ms(5), [&counter]() { ++counter;} );
//     ^~~~~~~~~~~~~~~~~~~~ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//TEST(TestRepeat, temp_object)
//{
//    unsigned int counter = 0;
//
//    asynchronous::repeat(ms(5), [&counter]() { ++counter;} );
//    std::this_thread::sleep_for(ms(20));
//
//    EXPECT_EQ(0, counter);
//}

TEST(TestRepeat, exception)
{
    asynchronous::latch started(1);
    auto g = asynchronous::repeat(ms(5), [&started]()
            {
                started.count_down();
                throw std::invalid_argument{"Hallo"};
            } );

    started.wait(); // make sure, we entered the loop once before we set the stop flag

    EXPECT_THROW(g.stop(), std::invalid_argument);
    EXPECT_NO_THROW(g.stop());
}

/**
 * This unit test shows what happens if we destroy the repeater before
 * it even was called
 * NOTE: disabled, because we can not guarantee, when which thread is running
 */
TEST(TestRepeat, DISABLED_interrupt)
{
    unsigned int counter = 0;
    asynchronous::latch done(100);
    {
        auto g = asynchronous::repeat(ms(20), increase, &counter, &done );
        std::this_thread::sleep_for(ms(5));
    }

    EXPECT_EQ(0u, counter);
}

//------------------------------------------------------------------------------
static void runWithWatchDogExpired()
{
    asynchronous::latch started(1);

    auto watchDog = asynchronous::repeat(ms(5), [&started] ()
            {
                started.count_down();
                // you might want to log here
                // the following exception is only to signal the "main" thread
                // this thread will end after the exception
                throw std::invalid_argument("Timeout");
            } );

    started.wait();
    watchDog.stop();   // to recognize the exception (if any)
}

static void runWithWatchDogInTime()
{
    auto watchDog = asynchronous::repeat(ms(20), [] () { throw std::invalid_argument("Timeout"); } );
    std::this_thread::sleep_for(ms(5));
    watchDog.stop();
}

/**
 * this unit test shows what happens if the first repetition throws already.
 * This is exactly what we think as a WatchDog.
 * NOTE: disabled, because we can not guarantee, when which thread is running
 */
TEST(TestRepeat, DISABLED_watchdog)
{
    EXPECT_THROW(runWithWatchDogExpired(), std::invalid_argument);
    EXPECT_NO_THROW(runWithWatchDogInTime());
}

//------------------------------------------------------------------------------
struct NumberQueue
{
    Mutex ivMutex;
    std::list<int> ivNumbers;

    explicit NumberQueue(std::list<int> numbers) :
        ivMutex(),
        ivNumbers(std::move(numbers))
    {}

    Lock getLock() { return Lock{ivMutex}; }

    bool isEmpty(const Lock&) const { return ivNumbers.empty(); }
    bool empty()
    {
        auto l = getLock();
        return isEmpty(l);
    }

    int pop(const Lock&)
    {
        auto result = ivNumbers.front();
        ivNumbers.pop_front();
        return result;
    }
};

static void pop_and_add(NumberQueue* numbers, int* sum)
{
    auto g = numbers->getLock();

    if (numbers->isEmpty(g)) return;

    *sum += numbers->pop(g);
    return;
}

TEST(TestRepeat, threadpool)
{
    constexpr size_t threadNumber = 3;

    const std::list<int> numbers = { 1,2,3,4,5,6,7,8,9,10 };
    NumberQueue numberQueue(numbers);
    int sum = 0;

    std::vector<asynchronous::repeat_impl::RepeatGuard> threads;

    threads.reserve(threadNumber);
    for(size_t i = 0; i < threadNumber; ++i) { threads.emplace_back(ms(0), pop_and_add, &numberQueue, &sum); }

    auto waiter = asynchronous::repeat(ms(0), &NumberQueue::empty, &numberQueue);
    waiter.wait();

    EXPECT_EQ(55, sum);
}

//******************************************************************************
// EOF
//******************************************************************************
