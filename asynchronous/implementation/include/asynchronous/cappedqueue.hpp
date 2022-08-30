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
// Created on: Jul 3, 2018
//     Author: oelsnerc, tseguine
//******************************************************************************

#pragma once

//******************************************************************************
#include "asynchronous/shared_resource.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <ostream>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * This implements a thread-safe FIFO queue with a maximum capacity.
 * Every item that is pushed into this queue, already at max cap, is dropped.
 * The managed type has to be default constructable.
 * The function "pop" is blocking ("try_pop" is not).
 */
template<typename T, size_t MAXSIZE>
class basic_capped_queue
{
public:
    using mutex_t = std::mutex;
    using lock_t = std::unique_lock<mutex_t>;

    using value_t      = T;
    using container_t  = std::queue<value_t>;

    using clock_t = std::chrono::steady_clock;
    using duration_t = clock_t::duration;
    using timepoint_t = clock_t::time_point;

    static constexpr size_t maxsize = MAXSIZE;

    //--------------------------------------------------------------------------
    struct PopResult
    {
        enum class State
        {
            unset, //!< the result is not set
            valid, //!< the result is valid and can be used
            empty, //!< the container is empty, and the result is default constructed
            timeout//!< the timed_pop reached a timeout, and the result is default constructed
        };

        template<typename STREAM>
        friend STREAM& printTo(STREAM& s, const State& result)
        {
            switch(result)
            {
            case State::unset : s << "unset"; break;
            case State::valid : s << "valid"; break;
            case State::empty : s << "empty"; break;
            case State::timeout : s << "timeout"; break;
            default : s << "unknown[" << static_cast<int>(result) << ']' ; break;
            }
            return s;
        }

        template<typename STREAM>
        friend STREAM& operator << (STREAM& s, const State& result)
        { return printTo(s, result); }

        // this overload is needed to calm down gtest's ambiguity
        friend std::ostream& operator << (std::ostream& s, const State& result)
        { return printTo(s, result); }

        State state;
        value_t  value;

        PopResult() :
            state(State::unset),
            value()
        {}

        explicit PopResult(State s) :
                state(std::move(s)),
                value()
        {}

        explicit PopResult(value_t v) :
                state(State::valid),
                value(std::move(v))
        {}

        bool isValid() const { return (state == State::valid); }
        explicit operator bool () const { return isValid(); }
    };

    using result_t = PopResult;

private:
    mutable mutex_t         ivMutex;
    std::condition_variable ivQueueCond;
    bool                    ivDone{false};
    size_t                  ivItemCount{0};
    size_t                  ivDroppedItemCount{0};
    container_t             ivQueue;

    /**
     * check if pop() should wait on the queue for further steps
     * @param synchronization object for this queue
     * @return true if the worker should wait
     */
    bool shouldWait(const lock_t& l) const
    {
        if (isDone(l)) return false;
        if (ivQueue.empty()) return true;
        return false;
    }

public:
    /**
     * @return synchronization object for this queue
     */
    lock_t getLock() const { return lock_t{ivMutex}; }

    /**
     * notify all consumers
     */
    void notify_all() { ivQueueCond.notify_all(); }

    /**
     * sets ivDone and notifies all consumers to end
     */
    void notifyToFinish()
    {
        { auto l = getLock(); ivDone = true; }
        notify_all();
    }

    /**
     * destructor: signal all consumers to end
     */
    ~basic_capped_queue() { notifyToFinish(); }

    //--------------------------------------------------------------------------
    /**
     * @return the next element in the queue
     *         if the queue is empty, it returns PopResult::State::empty
     * @param synchronization object for this queue
     */
    PopResult try_pop(const lock_t&)
    {
        if (ivQueue.empty()) { return PopResult{PopResult::State::empty}; }

        PopResult result{std::move(ivQueue.front())};
        ivQueue.pop();
        return result;
    }

    /**
     * returns the next element in the queue
     * if the queue is empty, it returns PopResult::State::empty
     */
    PopResult try_pop() { return try_pop(getLock()); }

    /**
     * blocks until at least one value was en-queued
     * or the queue is finished
     * @return the value, or PopResult::State::empty if the queue is finished
     */
    PopResult pop()
    {
        auto l = getLock();

        while( shouldWait(l) )
        {
            try { ivQueueCond.wait(l); }
            catch(...) {}
        }

        return try_pop(l);
    }

    /**
     * blocks until the queue is notified
     * @return the value, or PopResult::State::empty if the queue is finished
     */
    PopResult pop_unchecked()
    {
        auto l = getLock();

        if ( shouldWait(l) )
        {
            try { ivQueueCond.wait(l); }
            catch(...) {}
        }

        return try_pop(l);
    }

    /**
     * same as pop but times out when the timepoint is reached
     * @param timepoint
     * @return PopResult
     *          -> state = timeout if a timeout occurred
     */
    PopResult pop_wait_until(const timepoint_t& end)
    {
        auto l = getLock();
        while (shouldWait(l))
        {
            try
            {
                if (ivQueueCond.wait_until(l, end) ==  std::cv_status::timeout)
                { return PopResult{ PopResult::State::timeout }; }
            }
            catch(...) {}
        }
        return try_pop(l);
    }

    /**
     * same as pop but times out after duration
     * @param duration
     * @return PopResult
     *          -> state = timeout if a timeout occurred
     */
    PopResult pop_wait_for(const duration_t& duration)
    { return pop_wait_until( clock_t::now() + duration ); }

    /**
     * @return true if a new value was created and enqueued.
     * @return false if either the queue is finished or full
     *               in this case no value was created,
     *               but the droppedItemCount was increased;
     * in both cases the itemCount will be increased
     * @param synchronization object for this queue
     * @param args to the constructor of value
     */
    template<typename... ARGS>
    bool push_no_notify(const lock_t& l, ARGS&&... args)
    {
        ++ivItemCount;

        if (isDone(l) || full(l) )
        {
            ++ivDroppedItemCount;
            return false;
        }

        ivQueue.emplace(std::forward<ARGS>(args)...);
        return true;
    }

    /**
     * @return true if a new value was created and enqueued.
     *              in this case the ItemCount was increased
     *              and one consumer is notified
     * @return false if either the queue is finished or full
     *               in this case no value was created,
     *               but the droppedItemCount was increased;
     * @param args to the constructor of value
     */
    template<typename... ARGS>
    bool push(ARGS&&... args)
    {
        auto result = push_no_notify(getLock(),std::forward<ARGS>(args)...);
        ivQueueCond.notify_one();
        return result;
    }

    //--------------------------------------------------------------------------
    /**
     * provide the queue state functions
     */
    /**
     * @param synchronization object for this queue
     * @return true if the queue is closed for producers
     */
    bool isDone(const lock_t&) const { return ivDone; }

    /**
     * @param synchronization object for this queue
     * @return true if the max capacity is reached
     */
    bool full(const lock_t&) const
    {
        if (ivQueue.size() < maxsize) return false;
        return true;
    }

    bool empty(const lock_t&)  const { return ivQueue.empty(); }
    size_t size(const lock_t&) const { return ivQueue.size(); }

    size_t getItemCount(const lock_t&) const { return ivItemCount; }
    size_t getDroppedItemCount(const lock_t&) const { return ivDroppedItemCount; }

    //--------------------------------------------------------------------------
    bool isDone() const { return isDone(getLock()); }
    bool full()   const { return full(getLock()); }
    bool empty()  const { return empty(getLock()); }
    size_t size() const { return size(getLock()); }

    size_t getItemCount() const { return getItemCount(getLock()); }
    size_t getDroppedItemCount() const { return getDroppedItemCount(getLock()); }
};

//******************************************************************************

/**
 * This implements a thread-safe FIFO queue with a maximum capacity.
 * Every item that is pushed into a queue, already at max cap, is dropped.
 * The managed type has to be default constructable.
 * The function "pop" is blocking ("try_pop" is not).
 */
template<typename T, size_t MAXSIZE>
using CappedQueue = asynchronous::Reader< asynchronous::basic_capped_queue<T, MAXSIZE> >;

/**
 * implement a Reader/Writer interface for the capped queue
 * When the last writer is destroyed, all pop's return with State::empty
 */
template<typename T, size_t MAXSIZE>
using SharedCappedQueue = asynchronous::Writer< asynchronous::basic_capped_queue<T, MAXSIZE> >;

//------------------------------------------------------------------------------
template<typename Q>
using PopResult = typename Q::state_t::PopResult;

template<typename Q>
using PopState = typename Q::state_t::PopResult::State;

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
