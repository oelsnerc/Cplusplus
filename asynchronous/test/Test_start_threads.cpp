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
// Created on: Feb 27, 2019
//     Author: oelsnerc
//******************************************************************************

#include "asynchronous/start_threads.hpp"

#include <mutex>
#include <vector>
#include <map>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//******************************************************************************
static int add(int& a, const int& b)
{
    static std::mutex m;
    std::unique_lock<std::mutex> l(m);
    a+=b;
    return a;
}

TEST(Test_start_threads, simple_sum)
{
    constexpr int threadcount = 5;
    int a = 0;
    asynchronous::run_threads(threadcount, add, std::ref(a), 1);

    EXPECT_EQ( threadcount , a);
}

TEST(Test_start_threads, single_thread)
{
    constexpr int threadcount = 1;
    int a = 0;
    asynchronous::run_threads(threadcount, add, std::ref(a), 1);

    EXPECT_EQ( threadcount , a);
}

struct MyAdder
{
    mutable int sum;

    void add(const int& b)
    {
        ::add(sum, b);
    }

    void c_add(const int& b) const
    {
        ::add(sum, b);
    }

    int inc()
    {
        return sum++;
    }
};

TEST(Test_start_threads, mem_func)
{
    constexpr int threadcount = 5;
    MyAdder adder{0};

    asynchronous::run_threads(threadcount, &MyAdder::add, &adder, 1);

    EXPECT_EQ( threadcount , adder.sum);
}

TEST(Test_start_threads, mem_func_const)
{
    constexpr int threadcount = 5;
    const MyAdder adder{0};

    asynchronous::run_threads(threadcount, &MyAdder::c_add, &adder, 1);

    EXPECT_EQ( threadcount , adder.sum);
}

TEST(Test_start_threads, lambda)
{
    constexpr int threadcount = 5;
    int a = 0;

    asynchronous::run_threads( threadcount, [&a](const int& b) { add(a, b); }, 1);

    EXPECT_EQ( threadcount , a);
}

//------------------------------------------------------------------------------
TEST(Test_start_threads, queue_const)
{
    const std::vector<int> numbers = { 1,2,3,4,5 };
    int sum = 0;

    // NOTE: the '&' before add is needed, because w/o std::async does not like it
    asynchronous::for_each(2, numbers, &add, std::ref(sum));

    EXPECT_EQ(15, sum);
}

TEST(Test_start_threads, queue)
{
    std::vector<int> numbers = { 1,2,3,4,5 };

    asynchronous::for_each(2, numbers, [](int& a) { ++a; });

    const std::vector<int> expected = { 2, 3, 4, 5, 6 };
    EXPECT_EQ(expected, numbers);
}

TEST(Test_start_threads, queue_member)
{
    std::vector<int> numbers = { 1,2,3,4,5 };
    MyAdder adder{0};

    asynchronous::for_each(2, numbers, &MyAdder::add, &adder);

    EXPECT_EQ( 15 , adder.sum);
}

TEST(Test_start_threads, queue_obj_container)
{
    std::vector<MyAdder> adders =
    {
            MyAdder{1},
            MyAdder{2}
    };

    // call the same member function on different objects
    asynchronous::for_each(2, adders, &MyAdder::inc);

    EXPECT_EQ( 2 , adders[0].sum);
    EXPECT_EQ( 3 , adders[1].sum);
}

TEST( Test_start_threads, const_container_futures )
{
    const std::vector<int> numbers = { 1,2,3,4,5 };
    const int factor = 2;

    auto results = asynchronous::invoke_on_each(2, numbers,
                                                [](auto &a, int b)
                                                { return b*a; }
                                                , factor);

    int sum = 0;
    for(auto& result : results)
    {
        sum += result->get();
    }

    EXPECT_EQ( factor*15, sum);
}   // TEST const_container_futures

TEST( Test_start_threads, container_futures )
{
    std::vector<int> numbers = { 1,2,3,4,5 };

    auto results = asynchronous::invoke_on_each(2, numbers,
                                                [](int &a, int b)
                                                {
                                                    auto t = a;
                                                    a += b;
                                                    return t;
                                                }, 1);

    int sum = 0;
    for(auto& result : results)
    {
        sum += result->get();
    }

    EXPECT_EQ( 15, sum);
    EXPECT_EQ( (std::vector<int>{2,3,4,5,6}), numbers );
}   // TEST container_futures

TEST( Test_start_threads, container_future_memfunction )
{
    std::vector<MyAdder> adders =
            {
                    MyAdder{1},
                    MyAdder{2},
                    MyAdder{3},
                    MyAdder{4},
                    MyAdder{5}
            };

    // call the same member function on different objects
    auto results = asynchronous::invoke_on_each(2, adders, &MyAdder::inc);

    int sum = 0;
    for( auto& result : results)
    {
        sum += result->get();
    }

    EXPECT_EQ( 15, sum);

    int value = 2;
    for(auto& adder : adders)
    {
        EXPECT_EQ(value, adder.sum);
        ++value;
    }
}   // TEST container_future_memfunction

TEST( Test_start_threads, map_futures )
{
    std::map<int, std::string> values {
            { 1, ""},
            { 2, ""},
            { 3, ""},
            { 4, ""}
    };

    // the lambda returns void, so we're not interested in the actual results
    // we just want to wait until all elements were processed
    // NOTE: the destructor will wait for all threads to join
    asynchronous::invoke_on_each(2, values,
                                                [](auto& pair)
                                                {
                                                    if (pair.first % 2)
                                                    { pair.second = "yes"; };
                                                });

    for(auto& [index, string] : values)
    {
        if (index % 2)
        {
            EXPECT_EQ("yes", string);
        } else
        {
            EXPECT_TRUE(string.empty());
        }
    }

}   // TEST map_futures

//------------------------------------------------------------------------------
static std::atomic_size_t copyCounter;
static std::atomic_size_t moveCounter;

struct Multiplier
{
    int factor;

    explicit Multiplier(int f) : factor(f) {};

    Multiplier(const Multiplier& other) :
        factor(other.factor)
    { ++copyCounter; }

    Multiplier(Multiplier&& other) noexcept :
            factor(other.factor)
    {
        other.factor = 0;
        ++moveCounter;
    }

    void operator() (int& value) const
    {
        value *= factor;
    }
};

TEST( Test_start_threads, callableObjects_container_futures )
{
    constexpr size_t threadCount = 3;
    std::vector<int> numbers = { 1,2,3,4,5 };

    //--------------------------------------------------------------------------
    copyCounter = 0u;
    moveCounter = 0u;
    const Multiplier twice{2};

    // all threads use the same object
    asynchronous::invoke_on_each(threadCount, numbers, std::ref(twice));

    EXPECT_EQ( (std::vector<int>{2,4,6,8,10}), numbers );
    EXPECT_EQ(0u, copyCounter);
    EXPECT_EQ(0u, moveCounter);

    //--------------------------------------------------------------------------
    copyCounter = 0u;
    moveCounter = 0u;

    // each thread gets a copy of its own
    asynchronous::invoke_on_each(threadCount, numbers, Multiplier{3});

    EXPECT_EQ( (std::vector<int>{6,12,18,24,30}), numbers );
    EXPECT_EQ(threadCount, copyCounter);    // copied for each std::thread call
    EXPECT_EQ(0u, moveCounter);             // moved into each thread context

}   // TEST callableObjects_container_futures

TEST( Test_start_threads, startTooManyThreads )
{
    std::vector<int> numbers(1024, 1);
    auto results = asynchronous::invoke_on_each(numbers, [](const int& a) { return 2*a; });

    EXPECT_EQ(numbers.size(), results.size());

    EXPECT_GE(numbers.size(), results.threadCount());   // systems have a maximum available resource count
    EXPECT_LT(0u, results.threadCount());               // but at least one thread should be started!

    int sum = 0;
    for(auto& result : results)
    {
        sum += result->get();
    }

    EXPECT_EQ(2*numbers.size(), static_cast<size_t>(sum));

}   // TEST startTooManyThreads

TEST( Test_start_threads, carray )
{
    int numbers[] = { 1,2,3,4,5 };
    auto numberCount = sizeof(numbers)/sizeof(int);

    auto results = asynchronous::invoke_on_each(numbers, [](const int& a) { return 2*a; });

    EXPECT_EQ(numberCount, results.size());
    EXPECT_GE(numberCount, results.threadCount());  // systems have a maximum available resource count
    EXPECT_LT(0u, results.threadCount());           // but at least one thread should be started!

    int sum = 0;
    for(auto& result : results)
    {
        sum += result->get();
    }

    EXPECT_EQ(numberCount*(numberCount+1), static_cast<size_t>(sum));
}   // TEST carray

TEST( Test_start_threads, zeroThreads )
{
    int numbers[] = { 1,2,3,4,5 };

    auto results = asynchronous::invoke_on_each(0, numbers, [](const int& a) { return 2*a; });

    EXPECT_EQ(0u, results.threadCount());
    EXPECT_TRUE(results.empty());
}   // TEST zeroThreads

TEST( Test_start_threads, zeroValues )
{
    std::vector<int> numbers{};
    auto results = asynchronous::invoke_on_each(10, numbers, [](const int& a) { return 2*a; });

    EXPECT_EQ(0u, results.threadCount());
    EXPECT_TRUE(numbers.empty());
    EXPECT_TRUE(results.empty());
}   // TEST zeroValues

//******************************************************************************
// EOF
//******************************************************************************
