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
// Created on: Jul 24, 2018
//     Author: oelsnerc
//******************************************************************************

#include "asynchronous/onetimesignal.hpp"
#include "asynchronous/futurevalue.hpp"

#include <chrono>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//------------------------------------------------------------------------------
using ms = std::chrono::milliseconds;

//******************************************************************************
TEST(Test_OneTimeSignal, multiple_times)
{
    asynchronous::OneTimeSignal s;

    s.notify();

    EXPECT_FALSE(s.wait_for(ms(0)));
    EXPECT_FALSE(s.wait_for(ms(0)));

    s.notify();

    EXPECT_FALSE(s.wait_for(ms(0)));
}

TEST(Test_FutureValue, multiple_times)
{
    asynchronous::OneTimeFutureValue<int> s;

    EXPECT_NO_THROW(s.set_value(42));
    EXPECT_NO_THROW(s.set_value(5));

    EXPECT_EQ(42, s->get());
}

//******************************************************************************
// EOF
//******************************************************************************
