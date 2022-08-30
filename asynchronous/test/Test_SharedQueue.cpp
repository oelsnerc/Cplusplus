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
// Created on: Sep 17, 2018
//     Author: oelsnerc
//******************************************************************************

#include "asynchronous/queue.hpp"
#include "asynchronous/cappedqueue.hpp"
#include "asynchronous/latch.hpp"
#include "asynchronous/barrier.hpp"

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
TEST(Test_SharedQueue, simple)
{
    using value_t = std::string;
    using queue_t = asynchronous::SharedQueue<value_t>;

    asynchronous::latch ready(1);
    asynchronous::barrier producer_closed(2);
    std::ostringstream log;

    std::unique_ptr<queue_t> q{new queue_t()};
    auto consumer_task = std::async(std::launch::async, [&]()
            {
                auto consumer_side = q->as_reader();
                ready.count_down();
                producer_closed.count_down_and_wait();
                while(true)
                {
                    auto r = consumer_side->pop();
                    if (not r) break;
                    log << r.value;
                }
                EXPECT_EQ("Hello World", log.str());
            });

    EXPECT_TRUE((*q)->push("Hello"));
    EXPECT_TRUE((*q)->push(" "));
    EXPECT_TRUE((*q)->push("World"));

    //wait until the consumer side opens
    ready.wait();
    //destroy the producer side
    q.reset();
    //now the consumer side should still be able to pick up the messages and finish
    producer_closed.count_down_and_wait();
}

//------------------------------------------------------------------------------
TEST(Test_SharedQueue, detached)
{
    using value_t = std::string;
    using queue_t = asynchronous::SharedQueue<value_t>;

    using ms = std::chrono::milliseconds;

    asynchronous::latch done(1);
    std::ostringstream log;

    {
        queue_t q;
        EXPECT_TRUE(q->push("A "));
        auto consumer_side = q.as_reader();

        // create a copy of consumer_side inside the thread
        // so that we have now 2 consumers
        // Note: mutable is needed to have consumer_side non-const inside this block
        std::thread([&done, &log, consumer_side]() mutable
                {
                    while(true)
                    {
                        auto r = consumer_side->pop();
                        if (not r.isValid()) break;
                        log << r.value;
                    }
                    done.count_down();
                }).detach();
        //Make sure as best as we can that we are actually blocking in pop (Test will pass even if it doesn't)
        std::this_thread::sleep_for(ms{10});
        EXPECT_TRUE(q->push("detached"));
        EXPECT_TRUE(q->push(" Thread"));
    }

    done.wait();
    EXPECT_EQ("A detached Thread", log.str());
}

TEST(Test_SharedQueue, capping)
{
    using value_t = std::string;
    using queue_t = asynchronous::SharedCappedQueue<value_t, 3>;

    queue_t q;
    asynchronous::latch done(3);

    EXPECT_TRUE(q->push("Hello"));
    EXPECT_TRUE(q->push(" "));
    EXPECT_TRUE(q->push("World"));
    EXPECT_FALSE(q->push("1"));
    EXPECT_FALSE(q->push("2"));
    EXPECT_FALSE(q->push("3"));

    std::ostringstream log;
    std::thread([&]()
            {
                auto consumer_side = q.as_reader();
                while(true)
                {
                    auto r = consumer_side->pop();
                    if (not r) break;
                    log << r.value;
                    done.count_down();
                }
            }).detach();

    EXPECT_EQ(6u, q->getItemCount());
    EXPECT_EQ(3u, q->getDroppedItemCount());
    done.wait();
    EXPECT_EQ("Hello World", log.str());
}

TEST(Test_SharedQueue, lifetime)
{
    using value_t = std::string;
    using queue_t = asynchronous::SharedQueue<value_t>;
    using reader_t = typename queue_t::reader_t;

    std::unique_ptr<reader_t> consumer_side;
    {
        queue_t q;
        EXPECT_EQ(0u, q->getItemCount());
        EXPECT_TRUE(q->push("Hello"));
        EXPECT_EQ(1u, q->getItemCount());
        {
            auto r = q;
            EXPECT_EQ(1u, q->getItemCount());
            EXPECT_EQ(1u, r->getItemCount());
            EXPECT_FALSE(r->isDone());
            EXPECT_FALSE(q->isDone());
        }
        EXPECT_FALSE(q->isDone());
        EXPECT_EQ(1u, q->getItemCount());
        EXPECT_TRUE(q->push("World"));
        consumer_side.reset(new reader_t(q.as_reader()));
        {
            auto p = std::move(q);
            EXPECT_FALSE(p->isDone());
            EXPECT_FALSE((*consumer_side)->isDone());
        }
        EXPECT_TRUE((*consumer_side)->isDone());
    }
    EXPECT_TRUE((*consumer_side)->isDone());

    EXPECT_EQ(2u, (*consumer_side)->getItemCount());
    EXPECT_EQ(0u, (*consumer_side)->getDroppedItemCount());
    EXPECT_EQ("Hello", (*consumer_side)->pop().value);
    EXPECT_EQ("World", (*consumer_side)->pop().value);
}

/**
 * this is meant as a performance test case as well
 * on my system it takes 29ms, which is quite long!
 */
TEST(Test_SharedQueue, builtin_type)
{
    using value_t = size_t;
    using queue_t = asynchronous::SharedQueue<value_t>;
    using State = asynchronous::PopState<queue_t>;
    using ms = std::chrono::milliseconds;

    queue_t q;

    constexpr size_t number = 100000;
    for(size_t i = 1; i < number+1; ++i)
    { q->push(i); }

    size_t sum = 0;
    queue_t::result_t r;
    while( (r = q->pop_wait_for(ms{1})) )
    { sum += r.value; }

    EXPECT_EQ(State::timeout, r.state);
    EXPECT_EQ( (number*(number+1)/2) , sum);
}

TEST(Test_SharedQueue, unique_ptr)
{
    using value_t = std::unique_ptr<std::string>;
    using queue_t = asynchronous::SharedQueue<value_t>;
    using State = asynchronous::PopState<queue_t>;

    queue_t q;

    q->push(new std::string("Hello"));
    q->push(new std::string("World"));

    auto p1 = q->pop().value;
    auto p2 = q->pop().value;

    EXPECT_EQ(State::empty, q->try_pop().state);
    EXPECT_EQ( "Hello", *p1);
    EXPECT_EQ( "World", *p2);
}

//******************************************************************************
// EOF
//******************************************************************************
