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

#pragma once

//******************************************************************************
#include "asynchronous/doneflag.hpp"
#include "asynchronous/futurevalue.hpp"

//******************************************************************************
namespace asynchronous {
//******************************************************************************
/**
 * This class can be used to synchronize thread one time only
 * like:
 * 1 the main thread creates the OneTimeSignal
 * 2 the other thread calls wait_for on it
 * 3 the main thread calls notify on it
 * 4 the other will drop out of wait_for and can continue
 */
class OneTimeSignal
{
private:
    asynchronous::DoneFlag    ivDoneFlag;
    asynchronous::FutureValue<void> ivState;

public:
    OneTimeSignal() :
        ivDoneFlag(),
        ivState()
    {}

    /**
     * sleep at least for the given duration
     * @param duration
     * @return true on timeout
     */
    template<typename Duration>
    bool wait_for(const Duration& duration)
    {    // wait_for will not return "deferred" because the future is constructed that way.
         return (ivState->wait_for(duration) == std::future_status::timeout);
    }

    void notify()
    {
        if (not ivDoneFlag.set())
        { ivState.set_value(); }
    }
};

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
