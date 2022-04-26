/* begin copyright

 IBM Confidential

 Licensed Internal Code Source Materials

 3931, 3932 Licensed Internal Code

 (C) Copyright IBM Corp. 2022, 2022

 The source code for this program is not published or otherwise
 divested of its trade secrets, irrespective of what has
 been deposited with the U.S. Copyright Office.

 end copyright
*/

//******************************************************************************
// Created on: May 02, 2022
//     Author: oelsnerc
//******************************************************************************
#include "TermParser/termparser.hpp"

#include <sstream>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//******************************************************************************
namespace {
//------------------------------------------------------------------------------

static std::string toString(const mmc::Term& term)
{
    std::ostringstream s;
    s << term;
    return s.str();
}

//------------------------------------------------------------------------------
} // end of anonymous namespace
//------------------------------------------------------------------------------

TEST( Test_TermParser, simple )
{
    EXPECT_THROW(mmc::Term{""}, std::invalid_argument);

    EXPECT_EQ(1, mmc::Term{"1"}.calc());
    EXPECT_EQ(3, mmc::Term{"1+2"}.calc());
    EXPECT_EQ(6, mmc::Term{"2*3"}.calc());
    EXPECT_EQ(7, mmc::Term{"42/6"}.calc());
    EXPECT_EQ(13, mmc::Term{"2*6+1"}.calc());
    EXPECT_EQ(13, mmc::Term{"1+2*6"}.calc());
    EXPECT_EQ(25, mmc::Term{"1+2*3*4"}.calc());
    EXPECT_EQ(18, mmc::Term{"2*3+2*6"}.calc());
    EXPECT_EQ(19, mmc::Term{"2*3+2*6+1"}.calc());
    EXPECT_EQ(20, mmc::Term{"1+2*3+2*6+1"}.calc());
    EXPECT_EQ(10, mmc::Term{"1+2+3+4"}.calc());
    EXPECT_EQ(15, mmc::Term{"1+2+3*4"}.calc());

    EXPECT_EQ(42, mmc::Term{"+42"}.calc());
    EXPECT_EQ(-42, mmc::Term{"-42"}.calc());
    EXPECT_THROW(mmc::Term{"*42"}, std::invalid_argument);
    EXPECT_THROW(mmc::Term{"42+"}, std::invalid_argument);

    EXPECT_EQ(-41, mmc::Term{"-42+1"}.calc());
    EXPECT_EQ(41, mmc::Term{"+42-1"}.calc());

}   // TEST simple

TEST( Test_TermParser, parenthesis )
{
    EXPECT_EQ(2, mmc::Term{"(2)"}.calc());
    EXPECT_EQ(3, mmc::Term{"(1+2)"}.calc());
    EXPECT_EQ(9, mmc::Term{"3*(1+2)"}.calc());
    EXPECT_EQ(9, mmc::Term{"(1+2)*3"}.calc());
    EXPECT_EQ(12, mmc::Term{"(1+2)*(1+3)"}.calc());
    EXPECT_EQ(44, mmc::Term{"(1+(2*5))*(1+3)"}.calc());

    EXPECT_THROW(mmc::Term{"("}, std::invalid_argument);
    EXPECT_THROW(mmc::Term{"(1+2"}, std::invalid_argument);
    EXPECT_THROW(mmc::Term{")"}, std::invalid_argument);

}   // TEST parenthesis

TEST( Test_TermParser, function )
{
    EXPECT_EQ(4, mmc::Term{"sqr2"}.calc());
    EXPECT_EQ(4, mmc::Term{"sqr(2)"}.calc());
    EXPECT_EQ(5, mmc::Term{"1+sqr(2)"}.calc());
    EXPECT_EQ(5, mmc::Term{"sqr(2)+1"}.calc());
    EXPECT_EQ(19, mmc::Term{"1+2*sqr(1+2)"}.calc());

}   // TEST function

TEST( Test_TermParser, implicitMultiplication )
{
    EXPECT_EQ(6, mmc::Term{"2(1+2)"}.calc());
    EXPECT_EQ(7, mmc::Term{"1+2(1+2)"}.calc());
    EXPECT_EQ(19, mmc::Term{"1+2(1+2)*3"}.calc());
    EXPECT_EQ(18, mmc::Term{"2sqr(1+2)"}.calc());
    EXPECT_EQ(22, mmc::Term{"1+(1+2)(3+4)"}.calc());
    EXPECT_EQ(43, mmc::Term{"1+2(1+2)(3+4)"}.calc());

}   // TEST implicitMultiplication

TEST( Test_TermParser, power )
{
    EXPECT_EQ(16, mmc::Term{"2pow(2,3)"}.calc());
    EXPECT_EQ(64, mmc::Term{"sqr(pow(2,3))"}.calc());
    EXPECT_EQ(256, mmc::Term{"pow(2,3+5)"}.calc());
    EXPECT_EQ(32, mmc::Term{"pow(2,3)sqr(2)"}.calc());

    EXPECT_EQ(256, mmc::Term{"2^8"}.calc());
    EXPECT_EQ(16, mmc::Term{"2*2^(1+2)"}.calc());
    EXPECT_EQ(40320, mmc::Term{"2^3!"}.calc()); // 8!

}   // TEST power

TEST( Test_TermParser, faculty )
{
    EXPECT_EQ(120, mmc::Term{"5!"}.calc());
    EXPECT_EQ(120, mmc::Term{"(2+3)!"}.calc());
    EXPECT_EQ(12, mmc::Term{"2*3!"}.calc());
    EXPECT_EQ(12, mmc::Term{"3!*2"}.calc());

    EXPECT_EQ(12, mmc::Term{"2fac(3)"}.calc());
    EXPECT_EQ(720, mmc::Term{"fac(3)!"}.calc());    // 6!

}   // TEST faculty

TEST( Test_TermParser, average )
{
    EXPECT_EQ(2, mmc::Term{"avg(1,2,3)"}.calc());
    EXPECT_EQ(3, mmc::Term{"avg(3)"}.calc());

}   // TEST average

TEST( Test_TermParser, printing )
{
    EXPECT_EQ("(2*3)", toString(mmc::Term{"2*3"}));
    EXPECT_EQ("(fac(3)*(2+3))", toString(mmc::Term{"3!(2+3)"}));
    EXPECT_EQ("sqr((4/2))", toString(mmc::Term{"sqr(4/2)"}));
    EXPECT_EQ("pow(2,3)", toString(mmc::Term{"2^3"}));

}   // TEST printing

TEST( Test_TermParser, variables )
{
    const char* X_name = "x";

    mmc::setVariable(X_name, 42);

    EXPECT_EQ(42, mmc::getVariable(X_name));
}   // TEST variables

//------------------------------------------------------------------------------
TEST( Test_TermParser, perfomance )
{
    for(size_t i = 0; i < 10000; ++i)
    {
        ASSERT_EQ(19, mmc::Term{"1+2sqr(1+2)"}.calc());
    }
}   // TEST perfomance

TEST( Test_TermParser, sandbox )
{
    EXPECT_EQ("(fac(3)*(2+3))", toString(mmc::Term{"3!(2+3)"}));

}   // TEST sandbox

//******************************************************************************
// EOF
//******************************************************************************
