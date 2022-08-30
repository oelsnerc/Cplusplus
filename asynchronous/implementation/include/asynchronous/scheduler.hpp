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
// Created on: Sep 26, 2018
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * This implements a queue to schedule callbacks at a certain point in time.
 * It is only guaranteed that the callback is not called before that timepoint.
 * All callbacks are executed in the same thread, that means,
 * if a callback takes longer, the following (schedulewise) could miss their
 * wake up time.
 */
class Scheduler
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Callback = std::function<void(void)>;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    /**
     * the pair that holds the time point and the callback
     */
    struct Action
    {
        TimePoint   ivTimePoint;
        Callback    ivCallback;

        template<typename FUNC, typename... ARGS>
        explicit Action(TimePoint tp, FUNC&& func) :
            ivTimePoint(std::move(tp)),
            ivCallback(std::forward<FUNC>(func))
        {}

        // the priority_queue orders Big to Small
        // so we want the bigger timepoints to be at the end
        bool operator < (const Action& other) const
        { return ivTimePoint > other.ivTimePoint; }
    };

    using Queue = std::priority_queue<Action>;

private:
    mutable Mutex ivMutex;
    bool          ivDone = false;
    std::condition_variable ivCond;

    Queue       ivActions;
    std::thread ivThread;

    //--------------------------------------------------------------------------
    Lock getLock() const  { return Lock{ivMutex}; }

    //--------------------------------------------------------------------------
    // Worker thread functions

    /**
     * returns the next timpoint to wake up
     * @param synchronisation object
     * @return the timepoint to wait for
     */
    TimePoint getNextWakeUp(const Lock&)
    {
        if (ivActions.empty()) { return Clock::now() + std::chrono::hours{24}; }
        return ivActions.top().ivTimePoint;
    }

    /**
     * waits until either a new action was scheduled or
     * the wakeup time has passed for the next action in the scheduler
     * @param synchronisation object
     * @return true if a wakeup time was reached
     */
    bool wait(Lock& l)
    {
        while (true)
        {
            try
            {
                return (ivCond.wait_until(l, getNextWakeUp(l)) == std::cv_status::timeout);
            } catch(...) {}
        }
    }

    /**
     * executes the callback on top of the queue (if one exists)
     * NOTE: this does not hold the lock while we're executing the CALLBACK
     * NOTE: any exception thrown out of the execution of the callback is ignored.
     * @param synchronization object
     */
    void runTop(Lock& l)
    {
        if (ivDone) return;
        if (ivActions.empty()) return;

        auto func = std::move(ivActions.top().ivCallback);
        ivActions.pop();

        l.unlock();
        try  { func(); }
        catch(...) {}
        l.lock();
    }

    /**
     * the actual worker in the thread
     * it basically waits for anything to be scheduled or ready to be executed
     */
    void worker()
    {
        auto l = getLock();
        while (not ivDone)
        {
            if (wait(l))  { runTop(l); }
        }
    }

    //--------------------------------------------------------------------------

    /**
     * schedule this callback but do not notify the worker
     * @param tp
     * @param func
     */
    template<typename FUNC>
    void schedule_no_notify(TimePoint tp, FUNC&& func)
    {
        auto l = getLock();
        if (ivDone) return;

        if (not ivThread.joinable())
        { ivThread = std::thread(&Scheduler::worker, this); }

        ivActions.emplace(std::move(tp), std::forward<FUNC>(func));
    }

    /**
     * set ivDone to true
     * @return if the thread is running
     */
    bool prepareToFinish()
    {
        auto l = getLock();
        ivDone = true;
        return ivThread.joinable();
    }

public:
    ~Scheduler()
    {
        if (prepareToFinish())
        {
            ivCond.notify_all();
            ivThread.join();
        }
    }

    /**
     * clear the schedule, so that there are no callback to be scheduled
     * if a callback is running, it will not be interrupted
     */
    void clear()
    {
        {
            auto l = getLock();
            ivActions = Queue();
        }
        ivCond.notify_all();
    }

    /**
     * schedule the function to be called at this time point
     * @param tp
     * @param func
     */
    template<typename FUNC>
    void delay_until(TimePoint tp, FUNC&& func)
    {
        schedule_no_notify(std::move(tp), std::forward<FUNC>(func));
        ivCond.notify_all();
    }

    /**
     * schedule the function to be called not before this duration has passed
     * @param duration
     * @param func
     */
    template<typename FUNC>
    void delay_for(const Clock::duration& duration, FUNC&& func)
    {
        delay_until(Clock::now()+ duration, std::forward<FUNC>(func));
    }
};

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
