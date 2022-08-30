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
// Created on: Jul 4, 2018
//     Author: oelsnerc
//******************************************************************************

#include "asynchronous/queue.hpp"
#include "asynchronous/cappedqueue.hpp"
#include "asynchronous/latch.hpp"

#include <string>
#include <memory>
#include <future>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//******************************************************************************
using ms = std::chrono::milliseconds;

template<typename... ARGS>
inline std::future<void> start(ARGS&&... args)
{
    return std::async(std::launch::async, std::forward<ARGS>(args)...);
}

template<typename... ARGS>
inline std::future<void> defer(ARGS&&... args)
{
    return std::async(std::launch::deferred, std::forward<ARGS>(args)...);
}

template<typename T>
constexpr T sumUpTo(T v) { return v*(v+1)/2; }

//******************************************************************************
TEST(Test_Queue, simple)
{
    using value_t = std::string;
    using queue_t = asynchronous::Queue<value_t>;

    std::ostringstream log;

    queue_t q;
    auto consumer_task = start([&]()
            {
                while(auto r = q->pop())
                {
                    log << r.value;
                }
            });

    EXPECT_TRUE(q->push("Hello"));
    EXPECT_TRUE(q->push(" "));
    EXPECT_TRUE(q->push("World"));

    // check how many items have been in the queue
    EXPECT_EQ(3u, q->getItemCount());

    // the thread could have started already, so at most 3 items are in the queue
    EXPECT_GE(3u, q->size());

    // signal the consumer to finish when the q is empty
    q->notifyToFinish();

    // wait for the consumer to end
    consumer_task.get();

    EXPECT_EQ("Hello World", log.str());
    EXPECT_EQ(3u, q->getItemCount());
    EXPECT_EQ(0u, q->size());
    EXPECT_TRUE(q->empty());
}

TEST(Test_Queue, capped)
{
    using value_t = std::string;
    using queue_t = asynchronous::CappedQueue<value_t, 3>;

    std::ostringstream log;

    queue_t q;

    EXPECT_TRUE(q->push("Hello"));
    EXPECT_TRUE(q->push(" "));
    EXPECT_TRUE(q->push("World"));

    EXPECT_FALSE(q->push("ignored"));
    EXPECT_FALSE(q->push("still"));

    // check how many items have been in the queue
    EXPECT_EQ(5u, q->getItemCount());
    EXPECT_EQ(2u, q->getDroppedItemCount());
    EXPECT_EQ(3u, q->size());

    auto consumer_task = start([&]()
            {
                while(auto r = q->pop())
                {
                    log << r.value;
                }
            });

    // signal the consumer to finish when the q is empty
    q->notifyToFinish();

    // wait for the consumer to end
    consumer_task.get();

    EXPECT_EQ("Hello World", log.str());
    EXPECT_EQ(5u, q->getItemCount());
    EXPECT_EQ(2u, q->getDroppedItemCount());
    EXPECT_EQ(0u, q->size());
    EXPECT_FALSE(q->full());
    EXPECT_TRUE(q->empty());
}

//------------------------------------------------------------------------------
TEST(Test_Queue, detached)
{
    using value_t = std::string;
    using queue_t = asynchronous::Queue<value_t>;

    using ms = std::chrono::milliseconds;

    asynchronous::latch done(1);
    std::ostringstream log;

    queue_t q;
    EXPECT_TRUE(q->push("A "));
    std::thread([&]()
            {
                while(true)
                {
                    auto r = q->pop();
                    if (not r.isValid()) break;
                    log << r.value;
                }
                done.count_down();
            }).detach();

    //Make sure as best as we can that we are actually blocking in pop (Test will pass even if it doesn't)
    std::this_thread::sleep_for(ms{10});
    EXPECT_TRUE(q->push("detached"));
    EXPECT_TRUE(q->push(" Thread"));

    q->notifyToFinish();
    done.wait();
    EXPECT_EQ("A detached Thread", log.str());
}

TEST(Test_Queue, multipleProducerConsumer)
{
    using Workers = std::vector<std::future<void>>;
    using value_t = int;
    using queue_t = asynchronous::Queue<value_t>;

    std::atomic_int sum{0};
    queue_t q;


    constexpr int consumer_count = 10;
    constexpr int producer_count = 5;
    constexpr int producer_value = 10;

    Workers consumers;
    for (int i = 0; i < consumer_count; ++i)
    { consumers.emplace_back( start([&] { while(auto p = q->pop()) { sum+=p.value; }}) ); }

    Workers producers;
    for (int i = 0; i < producer_count; ++i)
    { producers.emplace_back( start([&] { for(int a = 0; a < producer_value; ++a) { q->push(a+1);} }) ); }

    // wait for all producers to be done
    producers.clear();

    // signal the consumers to finish, when the queue is empty
    q->notifyToFinish();

    // wait for all consumers to be done
    consumers.clear();

    // and check the sum
    constexpr int expected_sum = producer_count * sumUpTo(producer_value);
    EXPECT_EQ(expected_sum, sum);
}

//------------------------------------------------------------------------------
/**
 * this is meant as a performance test case as well
 * on my system it takes 29ms, which is quite long!
 */
TEST(Test_Queue, builtin_type)
{
    using value_t = size_t;
    using queue_t = asynchronous::Queue<value_t>;
    using PopResult = asynchronous::PopResult<queue_t>;
    constexpr size_t number = 100000;

    queue_t q;

    for(size_t i = 1; i < number+1; ++i)
    { q->push(i); }

    size_t sum = 0;
    PopResult r;
    while( (r = q->pop_wait_for(ms{1})) )
    { sum += r.value; }

    EXPECT_EQ(PopResult::State::timeout, r.state);
    EXPECT_EQ( sumUpTo(number) , sum);
}

// this takes only 2ms !!!
TEST(Test_Queue, builtin_type_no_lock)
{
    using value_t = size_t;
    using queue_t = asynchronous::Queue<value_t>;
    constexpr size_t number = 100000;

    queue_t q;

    {
        auto l = q->getLock();
        for(size_t i = 1; i < number+1; ++i)
        { q->push_no_notify(l,i); }
    }

    size_t sum = 0;

    {
        auto l = q->getLock();
        while (auto p = q->try_pop(l))
        { sum+=p.value; }
    }

    EXPECT_EQ( sumUpTo(number) , sum);
}

/**
 * how to deal with "non-copyable" objects
 */
TEST(Test_Queue, unique_ptr)
{
    using value_t = std::unique_ptr<std::string>;
    using queue_t = asynchronous::Queue<value_t>;
    using PopState = asynchronous::PopState<queue_t>;

    queue_t q;

    q->push(new std::string("Hello"));
    q->push(new std::string("World"));

    auto p1 = q->pop().value;
    auto p2 = q->pop().value;

    EXPECT_EQ(PopState::empty, q->try_pop().state);
    EXPECT_EQ( "Hello", *p1);
    EXPECT_EQ( "World", *p2);
}

//******************************************************************************
// EOF
//******************************************************************************
