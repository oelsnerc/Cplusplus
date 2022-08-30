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
// Created on: Jun 13, 2018
//     Author: boehme / seguine / oelsner
//******************************************************************************

#pragma once

//******************************************************************************
#include "asynchronous/onetimesignal.hpp"

#include <future>
#include <chrono>   // duration

#include <functional>  // invoke

//******************************************************************************
namespace asynchronous {
//******************************************************************************

//******************************************************************************
namespace repeat_impl {
//******************************************************************************
/**
 * helpers to convert a functions return value into bool
 * or return false if the return type is void
 */
template<typename T>
struct ConvertToBool
{
    template<typename... Args>
    bool operator () (Args&&... args) const
    {
        return static_cast<bool>( std::invoke(std::forward<Args>(args)...) );
    }
};

template<>
struct ConvertToBool<void>
{
    template<typename... Args>
    bool operator () (Args&&... args) const
    {
        std::invoke(std::forward<Args>(args)...);
        return false;
    }
};

template<typename... Args>
inline bool invokeAndConvertToBool(Args&&... args)
{
    using result_t = decltype(std::invoke(std::forward<Args>(args)...));
    const ConvertToBool<result_t> converter;
    return converter(std::forward<Args>(args)...);
}

//------------------------------------------------------------------------------
/**
 * This class starts a thread that calls
 * [action] in [duration] intervals
 *
 * On destruction the thread will be finished.
 * Any exception in the action will stop the thread and
 * will be thrown in the calling thread only when "stop" is called on this object
 * The destructor will not throw any exception
 *
 * If [action] does not return void, the return value will be casted into bool
 * and if the result is true the thread will end
 */
class RepeatGuard
{
private:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    OneTimeSignal      ivStopSignal;
    std::future<void>  ivFuture;

    /**
     * sleep the [duration] before calling [action] with the [args]
     * repeat that until exception is thrown in action
     * or
     * if [action] does not return void, the return value will be casted into bool
     * and if the result is true the thread will end
     *
     * @param duration
     * @param action
     * @param args
     */
    template <typename Action, typename... Args>
    void run(const Duration& duration, Action&& action, Args&&... args)
    {
        while(ivStopSignal.wait_for(duration))
        {   // an exception will end this thread
            // the future.get will throw it in the main thread (see "stop")
            // the future.wait will not throw, so the destructor will not throw either
            if (invokeAndConvertToBool(action, args...)) break;
        }
    }

    void finish() noexcept
    {
        if (not ivFuture.valid()) return;   // we might have been finished already
        ivStopSignal.notify();              // signal the action-thread to stop
        ivFuture.wait();                    // wait for the action-thread
    }

    template<typename T>
    using decay_t = typename std::decay<T>::type;

public:
    RepeatGuard(const RepeatGuard&) = delete;
    RepeatGuard& operator = (const RepeatGuard&) = delete;
    RepeatGuard(RepeatGuard&&) = default;
    RepeatGuard& operator = (RepeatGuard&&) = default;


    template <typename... Args>
    explicit RepeatGuard(const Duration& duration, Args&&... args) :
        ivStopSignal(),
        ivFuture( std::async(std::launch::async,
                &RepeatGuard::run< decay_t<Args>... >, this,
                duration,
                std::forward<Args>(args)...))
    {}

    ~RepeatGuard() noexcept { finish(); }

    /**
     * waits until the thread is finished
     * or will throw the exception from the action thread.
     */
    void wait()
    { if (ivFuture.valid()) { ivFuture.get(); } }

    /**
     * will signal the action thread to stop and wait for it
     * or will throw the exception from the action thread.
     */
    void stop() { finish();  wait(); }

};

//******************************************************************************
}  // namespace repeat_impl
//******************************************************************************

/**
 * calls [action] with [args] as parameters at an interval of [duration] in an separate thread
 *
 * the thread stays alive as long as the returned guard is alive
 * the thread waits the interval before calling [action]
 * the thread dies on the first exception in [action]
 *            or IF [action] does not return void,
 *               the return value will be casted into bool and
 *               if the result is true the thread will end
 *
 * the returned object controls the life-time of the thread.
 *     if the object is destroyed the thread will either wake up from the sleep and die
 *     or wait for return from [action] and die then.
 * calling "stop" and the returned object will end the thread and throw any exception
 *     that was thrown in [action]
 * the destructor of the returned object will not throw.
 *
 * @param duration - a time value to sleep between actions
 * @param action - the action to be called
 * @param args - provided as parameters to [action] in the same way as std::async does
 * @return a guard representing the life-time of the thread
 *         if you want any exception from [action] you can call "stop" on the guard
 */
template <typename Duration, typename Action, typename... Args>
[[nodiscard]] inline
repeat_impl::RepeatGuard repeat(Duration&& duration, Action&& action, Args&&... args)
{
    return repeat_impl::RepeatGuard{
            std::forward<Duration>(duration),
            std::forward<Action>(action),
            std::forward<Args>(args)...};
}

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
