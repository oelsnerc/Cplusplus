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
// Created on: Dec 5, 2019
//     Author: oelsnerc
//******************************************************************************

#include "asynchronous/lazy_thread_pool.hpp"
//#include "activemq/events/topic.hpp"

#include <atomic>
#include <thread>
#include <chrono>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//******************************************************************************
using INT = std::atomic_int;
static void add(INT& a, int b)
{
    a+=b;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST(Test_LazyThreadPool, simple)
{
    constexpr int expected_sum = 10;
    INT sum{0};

    {
        asynchronous::LazyThreadPool pool(4);

        for(int i = 0; i < expected_sum; ++i)
        { pool.addJob(add, std::ref(sum), 1); }
    }

    EXPECT_EQ(expected_sum, sum);
}

/**
 * This is an example for using the LazyThreadPool for
 * asynchronous event processing.
 * ActiveMQ call-backs should not take too long, because
 * AMQ is processing all call-backs in ONE single thread.
 * That means, if one call-back takes longer, all others have to wait.
 * The solution is to simply schedule threads in the call-back to do the actual job
 * And another good thing, the threads are only allocated when needed
 * and as soon as all messages are processed, the threads are "freed" again.
 * So the normal usage like "I'm waiting for a message that might come or not"
 * is a good usage for this LazyThreadPool
 */
//TEST(Test_LazyThreadPool, ActiveMQ)
//{
//    /**
//     * the function "add" waits 10ms, that means, if we call it 100 times
//     * we expect this test (w/o usage of a threadpool) to take > 1000ms
//     * if we allow a thread pool and use at most 10 threads,
//     * we expect something around 100ms
//     * NOTE: in both cases we expect some overhead for activeMQ and thread-management
//     *
//     * on my local system:
//     * w/o ThreadPool:  1380ms
//     * w/  ThreadPool:   264ms
//     */
//    constexpr int count = 100;
//    constexpr int expected_sum = count * (count+1) / 2;
//    INT sum{0};
//
//    {
//        asynchronous::LazyThreadPool pool(10);
//
//        se::events::Topic topic("abc");
//        auto subscription = topic.registerCallback<int>(
//                [&](const int& number)
//                {
//                    pool.addJob(add, std::ref(sum), number);    // takes ~ 264ms
//                    // add(sum, number);                        // takes ~1380ms
//                });
//
//        auto signal = topic.receiveSignal();  // special message used as a signal
//
//        for(int i = 0; i < count; ++i)
//        { topic.send(i+1); }
//
//        topic.sendSignal();     // last message
//        signal->get();          // wait until all messages are delivered by AMQ
//    }   // wait until all threads are finished
//
//    EXPECT_EQ(expected_sum, sum);
//}

//******************************************************************************
// EOF
//******************************************************************************
