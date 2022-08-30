/* begin copyright

   IBM Confidential

   Licensed Internal Code Source Materials

   3931, 3932 Licensed Internal Code

   (C) Copyright IBM Corp. 2015, 2022

   The source code for this program is not published or otherwise
   divested of its trade secrets, irrespective of what has
   been deposited with the U.S. Copyright Office.

   end copyright
*/

//******************************************************************************
#include "asynchronous/barrier.hpp"
#include <chrono>

#include <thread>
#include <string>
#include <list>
#include <functional>
#include <memory>
#include <gtest/gtest.h>

// #include "trace/trace.hpp"
// CREATE_TRACESTREAM("ASYNC");
#define FENTRY(...) {}
#define FTRACE(...) {}
#define FTRACEF(...) {}
#define FEXIT(...) {}

//******************************************************************************
void WaitAndSet42(asynchronous::barrier& Bar, int& VerifyValue)
{
    FENTRY();
    Bar.count_down_and_wait();

    FTRACE("Set Verify");
    VerifyValue = 42;

    Bar.count_down_and_wait();
    FEXIT();
}

TEST(Test_barrier, Basic)
{
    FENTRY();
    asynchronous::barrier b(2);
    int a = 0;

    std::thread set42(WaitAndSet42, std::ref(b), std::ref(a));

    ASSERT_EQ(0, a);

    FTRACE("Release the barrier for the first time");
    b.count_down_and_wait();    // see the first count_down_and_wait in WaitAndSet42

    FTRACE("Release the barrier for the 2nd time");
    b.count_down_and_wait();    // see the 2nd count_down_and_wait in WaitAndSet42

    ASSERT_EQ(42, a);

    set42.join();

    FEXIT();
}

//------------------------------------------------------------------------------
void Set42(int& VerifyValue)
{
    FENTRY(VerifyValue);
    VerifyValue = 42;
    FEXIT(VerifyValue);
}

TEST(Test_barrier, Release)
{
    FENTRY();

    int a = 0;

    asynchronous::barrier b(1, [&a](const size_t& count) { Set42(a); return count;});

    ASSERT_EQ(0, a);

    b.count_down_and_wait();

    ASSERT_EQ(42, a);
    FEXIT();
}

//------------------------------------------------------------------------------
void WaitForGo(asynchronous::barrier& Bar)
{
    FENTRY();
    Bar.count_down_and_wait();
    FEXIT();    // we had a couple of review builds, where this trace was
                // hanging on the vfprintf ... maybe the cout was locked?
}

asynchronous::barrier::value_t ResetFunc(const asynchronous::barrier::value_t& resetCount)
{
    static asynchronous::barrier::value_t count = resetCount;
    FTRACEF(count);
    return ++count;
}

TEST(Test_barrier, Reset)
{
    FENTRY();
    asynchronous::barrier::value_t resetCount = 1;
    asynchronous::barrier b(ResetFunc(resetCount), ResetFunc);

    ASSERT_EQ(2u, b.getResetCount());

    std::list<std::thread> threads;

    FTRACEF("Create one other thread");
    threads.emplace_back(WaitForGo, std::ref(b));

    FTRACEF("Release the barrier for the first time");
    b.count_down_and_wait();

    ASSERT_EQ(3u, b.getResetCount());

    FTRACEF("Create 2 other threads");
    threads.emplace_back(WaitForGo, std::ref(b));
    threads.emplace_back(WaitForGo, std::ref(b));

    FTRACEF("Release the barrier for the 2nd time");
    b.count_down_and_wait();

    ASSERT_EQ(4u, b.getResetCount());

    FTRACEF("Wait for threads to join again");
    for(auto& t : threads) { t.join(); }

    FEXIT();
}

//------------------------------------------------------------------------------
class MyResetClass
{
    std::string ivName;

    using value_t = asynchronous::barrier::value_t;

public:
    explicit MyResetClass(const std::string& name) : ivName(name) {}

    value_t doReset(const value_t& init) const
    {
        return init;
    }
};

TEST(Test_barrier, ClassAndBind)
{
    using namespace std::placeholders;

    MyResetClass reset("TestMe");

    asynchronous::barrier b(1, std::bind(&MyResetClass::doReset, reset, _1));
    b.count_down_and_wait();
    ASSERT_EQ(1u, b.getResetCount());
}

TEST(Test_barrier, ClassAndLambda)
{
    using namespace std::placeholders;
    using namespace asynchronous;

    MyResetClass reset("TestMe");

    asynchronous::barrier b(1, [&reset] (const barrier::value_t& count) { return reset.doReset(count); });
    b.count_down_and_wait();
    ASSERT_EQ(1u, b.getResetCount());
}

TEST(Test_barrier, UniquePtr)
{
    std::unique_ptr<asynchronous::barrier> b(new asynchronous::barrier(1));
    ASSERT_TRUE(b);

    b->count_down_and_wait();
    EXPECT_EQ(1u, b->getResetCount());
}
