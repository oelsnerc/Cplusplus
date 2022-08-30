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
#include "asynchronous/latch.hpp"
#include <chrono>

#include <thread>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//******************************************************************************
TEST(Test_latch, CountDown)
{
    asynchronous::latch l(1);

    EXPECT_FALSE(l.try_wait());
    EXPECT_FALSE(l.wait_for(std::chrono::microseconds{1}));

    l.count_down();

    EXPECT_TRUE(l.wait_for(std::chrono::microseconds{1}));
    EXPECT_TRUE(l.try_wait());
}

//------------------------------------------------------------------------------
void DecreaseLatch(asynchronous::latch& Latch)
{
    Latch.count_down();
}

TEST(Test_latch, Wait)
{
    asynchronous::latch l(2);

    std::thread t1(DecreaseLatch, std::ref(l));
    std::thread t2(DecreaseLatch, std::ref(l));

    l.wait();

    EXPECT_TRUE(l.try_wait());

    t1.join();
    t2.join();
}

//------------------------------------------------------------------------------
void DecreaseLatchAndWait(asynchronous::latch& Latch)
{
    Latch.count_down_and_wait();
}

TEST(Test_latch, CountDownAndWait)
{
    try
    {
        asynchronous::latch l(2);

        std::thread t1(DecreaseLatchAndWait, std::ref(l));

        EXPECT_FALSE(l.try_wait());

        std::thread t2(DecreaseLatchAndWait, std::ref(l));

        t1.join();
        t2.join();

        EXPECT_TRUE(l.try_wait());
    } catch(std::exception & e)
    {
        FAIL() << e.what();
    }
}

//******************************************************************************
// EOF
//******************************************************************************
