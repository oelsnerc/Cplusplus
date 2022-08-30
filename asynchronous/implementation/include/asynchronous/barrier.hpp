/*
 IBM Confidential

 Licensed Internal Code Source Materials

 3931, 3932 Licensed Internal Code

 (C) Copyright IBM Corp. 2015, 2022

 The source code for this program is not published or otherwise
 divested of its trade secrets, irrespective of what has
 been deposited with the U.S. Copyright Office.
*/

//******************************************************************************
#pragma once

//******************************************************************************
#include "asynchronous/waiter.hpp"
#include <functional>

//******************************************************************************
namespace asynchronous {
//******************************************************************************
/**
 * A barrier maintains an internal thread counter that is initialized when
 * the barrier is created. Threads may decrement the counter and then block
 * waiting until the specified number of threads are blocked.
 * All threads will then be woken up, and the barrier will reset.
 * In addition, there is a mechanism to change the thread count value
 * after the count reaches 0.
 * see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3666.html
 */

class barrier : private asynchronous::WaiterForZero<size_t>
{
public:
    using base_t = asynchronous::WaiterForZero<size_t>;
    using value_t = base_t::value_t;
    using reset_func_t = std::function<value_t(const value_t&)>;

private:
    value_t         ivResetCount;
    reset_func_t    ivResetFunc;

    //--------------------------------------------------------------------------
    /**
     * called when the internal thread count reaches 0
     * it calls the Completion function and the reset function
     * and resets the internal thread count and changes the thread count generation
     */
    void reset(lock_t& l)
    {
        ivResetCount = ivResetFunc(ivResetCount);
        locked_setValue(l, ivResetCount);
    }

public:
    /**
      * create a barrier
      * The value is the initial value of the barrier
      * Throws an std::invalid_argument if the value is 0
     * @param Value
     */
    explicit barrier(value_t Value, reset_func_t Func) :
        base_t{Value},
        ivResetCount(Value),
        ivResetFunc(std::move(Func))
    {
        if (try_wait()) throw std::invalid_argument("Barrier created with a thread count of 0");
    }

    /**
      * create a barrier
      * The value is the initial value of the barrier
      * Throws an std::invalid_argument if the value is 0
     * @param Value
     */
    explicit barrier(value_t Value) : barrier(Value, [](const value_t& count) { return count;})
    { }

    //--------------------------------------------------------------------------
    /**
     * @brief Decrements the internal thread count by 1.
     *        If the resulting count is not 0, blocks the calling thread
     *        until the internal count is decremented to 0
     *        by one or more other threads calling count_down_and_wait().
     *        or a timeout occurred
     * @param timepoint when the timeout should occur
     * @return false if a timeout occurred
     */
    bool count_down_and_wait_until(const base_t::timepoint_t& timepoint)
    {
        auto l = check();
        if (locked_count_down(l))
        {
            reset(l);
            return true;
        }
        return base_t::locked_wait_until(l, timepoint);
    }

    /**
     * @brief Decrements the internal thread count by 1.
     *        If the resulting count is not 0, blocks the calling thread
     *        until the internal count is decremented to 0
     *        by one or more other threads calling count_down_and_wait().
     *        or a timeout occurred
     * @param duration when the timeout should occur
     * @return false if a timeout occurred
     */
    bool count_down_and_wait_for(const base_t::duration_t& duration)
    { return count_down_and_wait_until(base_t::clock_t::now() + duration); }

    /**
     * Decrements the internal thread count by 1.
     * If the resulting count is not 0, blocks the calling thread
     * until the internal count is decremented to 0
     * by one or more other threads calling count_down_and_wait().
     */
    void count_down_and_wait()
    { count_down_and_wait_until(getInfinity()); }

    /**
     * @return the current reset thread count
     */
    value_t getResetCount() const
    {
        auto l = getLock();
        return ivResetCount;
    }
};

//******************************************************************************
} // end of namespace asynchronous
//******************************************************************************
