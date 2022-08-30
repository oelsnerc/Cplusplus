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
// Created on: Sep 6, 2018
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * This class contains a value and a predicate for this value
 * and allows to wait on a modification of that value,
 * that makes the predicate return true.
 *
 */
template<typename T, typename PREDICATE>
class WaiterImpl
{
public:
    using mutex_t = std::mutex;
    using lock_t  = std::unique_lock<mutex_t>;
    using mutex_ptr_t = std::unique_ptr<mutex_t>;
    using cond_t = std::condition_variable;
    using cond_ptr_t = std::unique_ptr<cond_t>;
    using value_t = T;
    using pred_t  = PREDICATE;
    using clock_t = std::chrono::steady_clock;
    using duration_t = clock_t::duration;
    using timepoint_t = clock_t::time_point;

private:
    /**
     * Provide a "guard" object, that holds the lock while existing
     * Through this guard one can access the value in the "usual" way
     * it helps to see this as a "pointer" to value
     */
    class ConstUpdater
    {
    protected:
        lock_t            ivLock;
        const WaiterImpl* ivWaiterPtr;

        friend class WaiterImpl;

        /**
         * Create the guard object to enable easy synchronized access
         * but create it only by this SynchronizedValue
         * @param MutexRef the mutex to be locked while we exist
         * @param ValuePtr the value we want to grant access to
         */
        ConstUpdater(lock_t lock, const WaiterImpl* waiter) :
            ivLock(std::move(lock)),
            ivWaiterPtr(waiter)
        {}

        const value_t* getValue() const { return &(ivWaiterPtr->ivValue); }

    public:
        ConstUpdater(const ConstUpdater& other) = delete;
        ConstUpdater& operator = (const ConstUpdater& other) = delete;

        ConstUpdater(ConstUpdater&& other) :
            ivLock(std::move(other.ivLock)),
            ivWaiterPtr(other.ivWaiterPtr)
        {
            other.ivWaiterPtr = nullptr;
        }

        ConstUpdater& operator = (ConstUpdater&& other)
        {
            if (&other != this)
            {
                ivLock = std::move(other.ivLock);
                ivWaiterPtr = other.ivWaiterPtr;
                other.ivWaiterPtr = NULL;
            }
            return *this;
        }

        /**
         * cast operator to return a copy of the value
         */
        operator value_t () const { return *getValue(); }

        /**
         * pointer access operator to return a pointer to the value
         * this enables the chaining of "->" to let the compiler
         * collapse all "->" into only one.
         * @return pointer to the value
         */
        const value_t* operator -> () const { return getValue(); }

        /**
         * pointer dereference operator to a return a ref to the value
         * @return reference to the value
         */
        const value_t& operator * () const { return *getValue(); }

        /**
         * Enable direct compare to the value
         * @param other can be of any type that ValueType implements "==" for
         * @return whatever "==" returns converted to bool
         */
        template<typename OtherType>
        bool operator == (OtherType&& other) const
        { return (*getValue() == std::forward<OtherType>(other)); }
    };

    class Updater : public ConstUpdater
    {
    private:
        friend class WaiterImpl;

        using ConstUpdater::ConstUpdater;

        WaiterImpl* getWaiter() { return const_cast<WaiterImpl*>(this->ivWaiterPtr); }
        value_t* getValue() { return &(getWaiter()->ivValue); }

    public:
        ~Updater()
        {
            if (this->ivWaiterPtr)
            { getWaiter()->checkAndNotify(this->ivLock); }
        }

        Updater(Updater&& other) : ConstUpdater(std::move(other)) {}

        Updater& operator = (Updater&& other)
        {
            ConstUpdater::operator =(std::move(other));
            return *this;
        }

        /**
         * pointer access operator to return a pointer to the value
         * this enables the chaining of "->" to let the compiler
         * collapse all "->" into only one.
         * @return pointer to the value
         */
        value_t* operator -> () { return getValue(); }

        /**
         * pointer dereference operator to a return a ref to the value
         * @return reference to the value
         */
        value_t& operator * () { return *getValue(); }

        /**
         * Enable the direct assignment of the value
         * @param other can be of any type that ValueType implements "=" for
         * @return reference to this updater (to enable chaining)
         */
        template<typename OtherType>
        Updater& operator = (OtherType&& other)
        {
            *getValue() = std::forward<OtherType>(other);
            return *this;
        }
    };

private:
    mutex_ptr_t  ivMutex;
    cond_ptr_t   ivCondition;
    value_t      ivValue;
    pred_t       ivPredicate;

    /**
     * the following is a circumvention
     * for the "spurious wakeups" of the condition variable.
     * we simply count how many times the predicate returned true after a modification
     * and if this count has not changed while we were waiting,
     * this is a "spurious wake up" and we have to wait again
     */
    unsigned int ivGeneration;

    /**
     * @param lock to synchronize this object
     * @return the result of pred_t::test
     */
    bool testPredicate(const lock_t&) const
    { return static_cast<bool>(ivPredicate.test(ivValue)); }

    /**
     *
     * @param lock to synchronize this object
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     * @return the result of the setup function
     */
    template<typename... ARGS>
    bool setupPredicate(const lock_t&, ARGS&&... args)
    { return static_cast<bool>(ivPredicate.setup(ivValue, std::forward<ARGS>(args)...)); }

protected:
    lock_t getLock() const { return lock_t(*ivMutex); }

    /**
     * @brief seems like the condition_variable does not wait for timepoint::max()
     *        so we need a point far in the future (100 years seems fair)
     * @return the maximum timepoint to wait on
     */
    static auto getInfinity()
    {
        constexpr auto year = std::chrono::hours{24} * 365;
        return clock_t::now() + 100*year;
    }

    /**
     * check if the predicate returns true
     * and if so notify the condition variable
     * @param lock to synchronize this object
     * @return the result of the predicate
     */
    bool checkAndNotify(const lock_t& l)
    {
        if (testPredicate(l))
        {
            ++ivGeneration;
            ivCondition->notify_all();
            return true;
        }
        return false;
    }

    /**
     * make sure we set the value only under lock
     * @param lock to synchronize this object
     * @param value
     */
    template<typename VALUE>
    void locked_setValue(const lock_t& l, VALUE&& value)
    {
        ivValue = std::forward<VALUE>(value);
        checkAndNotify(l);
    }

    const value_t& locked_getValue(const lock_t&) const
    { return ivValue; }

    /**
     * Returns true if the predicate returned true, and false otherwise.
     * Does not block the calling thread.
     * @param lock to synchronize this object
     * @return what the predicate returned
     */
    template<typename... ARGS>
    bool locked_try_wait(const lock_t& l, ARGS&&... args)
    { return setupPredicate(l, std::forward<ARGS>(args)...); }

    /**
     * blocks the calling thread until the predicate returns true
     * or a timeout occurs.
     * if the predicate returns true already, this is a no-op
     * @param lock to synchronize this object
     * @param timepoint when the timeout should occur
     * @return false if timeout has occurred.
     */
    template<typename... ARGS>
    bool locked_wait_until(lock_t& lock, const timepoint_t& timepoint, ARGS&&... args)
    {
        if (locked_try_wait(lock, std::forward<ARGS>(args)...)) { return true; }

        return ivCondition->wait_until(lock, timepoint,
                   [myGeneration = ivGeneration, this]()
                   { return ivGeneration != myGeneration; }
        );
    }

    /**
     * blocks the calling thread until the predicate returns true
     * if the predicate returns true already, this is a no-op
     * @param lock to synchronize this object
     */
    template<typename... ARGS>
    void locked_wait(lock_t& lock, ARGS&&... args)
    { locked_wait_until(lock, getInfinity(), std::forward<ARGS>(args)...); }

    /**
     * run the callback to modify the value and
     * check the predicate after that.
     * if the predicate returned true, all waiters are notified
     * @param lock to synchronize this object
     * @param callback
     * @return what the predicate returned
     */
    template<typename CALLBACK>
    bool locked_modify(const lock_t& l, CALLBACK&& callback)
    {
        std::forward<CALLBACK>(callback)(ivValue);
        return checkAndNotify(l);
    }

    /**
     * run the callback to modify the value and wait for the
     * predicate to return true or a timeout occurred.
     * @param lock to synchronize this object
     * @param timepoint when the timeout should occur
     * @param callback
     * @return false if a timeout occurred
     */
    template<typename CALLBACK, typename... ARGS>
    bool locked_modify_and_wait_until(lock_t& l, const timepoint_t& timepoint,
                                      CALLBACK&& callback, ARGS&&... args)
    {
        locked_modify(l, std::forward<CALLBACK>(callback));
        return locked_wait_until(l, timepoint, std::forward<ARGS>(args)...);
    }

    /**
     * run the callback to modify the value and wait for the
     * predicate to return true
     * @param lock to synchronize this object
     * @param callback
     */
    template<typename CALLBACK, typename... ARGS>
    void locked_modify_and_wait(lock_t& l, CALLBACK&& callback, ARGS&&... args)
    {
        locked_modify_and_wait_until(l, getInfinity(),
                                     std::forward<CALLBACK>(callback),
                                     std::forward<ARGS>(args)...);
    }

public:
    explicit WaiterImpl(value_t value, pred_t predicate) :
        ivMutex(new mutex_t()),
        ivCondition(new cond_t()),
        ivValue(std::move(value)),
        ivPredicate(std::move(predicate)),
        ivGeneration(0)
    {}

    explicit WaiterImpl(value_t value) : WaiterImpl(std::move(value), pred_t{})
    {}

    explicit WaiterImpl(pred_t predicate) : WaiterImpl(value_t{}, std::move(predicate))
    {}

    explicit WaiterImpl() : WaiterImpl(value_t{}, pred_t{})
    {}

    /**
     * check if the predicate returns true currently
     * this call is not blocking
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     * @return the result of the predicate
     */
    template<typename... ARGS>
    bool try_wait(ARGS&&... args)
    { return locked_try_wait(getLock(), std::forward<ARGS>(args)...); }

    /**
     * blocks the calling thread until the predicate returns true
     * if the predicate returns true already, this is a no-op
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     */
    template<typename... ARGS>
    void wait(ARGS&&... args)
    {
        auto l = getLock();
        locked_wait(l, std::forward<ARGS>(args)...);
    }

    /**
     * blocks the calling thread until the predicate returns true
     * or a timeout occurred
     * if the predicate returns true already, this is a no-op
     * @param timepoint when the timeout should occur
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     * @return false if a timeout occurred
     */
    template<typename... ARGS>
    bool wait_until(const timepoint_t& timepoint, ARGS&&... args)
    {
        auto l = getLock();
        return locked_wait_until(l, timepoint, std::forward<ARGS>(args)...);
    }

    /**
     * blocks the calling thread until the predicate returns true
     * or a timeout occurred
     * if the predicate returns true already, this is a no-op
     * @param duration when the timeout should occur
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     * @return false if a timeout occurred
     */
    template<typename... ARGS>
    bool wait_for(const duration_t& duration, ARGS&&... args)
    { return wait_until(clock_t::now() + duration, std::forward<ARGS>(args)...); }

    /**
     * run the callback to modify the value and
     * check the predicate after that.
     * if the predicate returned true, all waiters are notified
     * @param callback
     * @return what the predicate returned
     */
    template<typename CALLBACK>
    bool modify(CALLBACK&& callback)
    { return locked_modify(getLock(), std::forward<CALLBACK>(callback)); }

    /**
     * run the callback to modify the value and wait for the
     * predicate to return true or a timeout occurred
     * @param timepoint when the timeout should occur
     * @param callback
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     * @return false if a timeout occurred
     */
    template<typename CALLBACK, typename... ARGS>
    bool modify_and_wait_until(const timepoint_t& timepoint,
                               CALLBACK&& callback,
                               ARGS&&... args)
    {
        auto l = getLock();
        return locked_modify_and_wait_until(l, timepoint,
                                            std::forward<CALLBACK>(callback),
                                            std::forward<ARGS>(args)...);
    }

    /**
     * run the callback to modify the value and wait for the
     * predicate to return true or a timeout occurred
     * @param duration when the timeout should occur
     * @param callback
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     * @return false if a timeout occurred
     */
    template<typename CALLBACK, typename... ARGS>
    bool modify_and_wait_for(const duration_t& duration,
                               CALLBACK&& callback,
                               ARGS&&... args)
    {
        return modify_and_wait_until(clock_t::now() + duration,
                                     std::forward<CALLBACK>(callback),
                                     std::forward<ARGS>(args)...);
    }

    /**
     * run the callback to modify the value and wait for the
     * predicate to return true
     * @param callback
     * @param args additional parameters to the setup functions
     *        NOTE: the first parameter is the current value of this Waiter
     */
    template<typename CALLBACK, typename... ARGS>
    void modify_and_wait(CALLBACK&& callback, ARGS&&... args)
    {
        auto l = getLock();
        locked_modify_and_wait(l, std::forward<CALLBACK>(callback), std::forward<ARGS>(args)...);
    }

    /**
     * call the "operator =" with the new value and
     * notify the waiters, if the predicate returned true afterwards
     * @param value
     */
    template<typename VALUE>
    void setValue(VALUE&& value)
    { locked_setValue(getLock(), std::forward<VALUE>(value)); }

    /**
     * @return a copy of the curent value
     */
    value_t getValue() const
    { return locked_getValue(getLock()); }

    /**
     * Create an update object that holds the lock while existing.
     * @return an guarding object providing synchronized access to the value
     */
    Updater getUpdater() { return Updater(getLock(), this); }
    ConstUpdater getUpdater() const { return ConstUpdater(getLock(), this); }

    /**
     * dereference operator to return a guarding updater object
     * @return an guarding object providing synchronized access to the value
     */
    Updater operator* () { return getUpdater(); }
    ConstUpdater operator* () const { return getUpdater(); }

    /**
     * access operator to return a guarding updater object
     * @return an guarding object providing synchronized access to the value
     */
    Updater operator -> () { return getUpdater(); }
    ConstUpdater operator -> () const { return getUpdater(); }
};

/**
 * to avoid any syntax sugar, like "reference" or "const"
 * we have to decay the type and use this as the value type in the waiter
 */
template<typename T, typename PREDICATE>
using Waiter = WaiterImpl< typename std::decay<T>::type, PREDICATE>;

/**
 * convenient function to create a waiter for this value and this predicate
 * @param value
 * @param predicate
 * @return thr waiter object initialized with this value
 */
template<typename T, typename PREDICATE>
inline Waiter<T, PREDICATE> createWaiter(T&& value, PREDICATE&& predicate)
{
    return Waiter<T, PREDICATE>(std::forward<T>(value), std::forward<PREDICATE>(predicate));
}

//******************************************************************************
namespace checker {
//******************************************************************************
/**
 * the idea here is, that the first time this checker is called
 * it is the entry of "wait" (so it has to return false)
 * the 2nd time is after the modification (so it has to return true)
 * If we call "wait" again, we want it to start over
 * NOTE: both calls are done under a lock
 */
template<typename T>
struct HasChangedImpl
{
    T value;

    template<typename K>
    bool test (K&& k) const
    { return (value != std::forward<K>(k)); }

    template<typename K>
    bool setup (K&& k)
    {
        value = std::forward<K>(k);
        return false;
    }

};

template<typename T>
using HasChanged = HasChangedImpl< std::decay_t<T> >;

//------------------------------------------------------------------------------
template<typename T>
struct AtLeastImpl
{
    T value;

    template<typename K>
    bool test (K&& k) const
    { return (value <= std::forward<K>(k)); }

    template<typename K, typename L>
    bool setup (K&& k, L&& l)
    {
        value = std::forward<L>(l);
        return test(std::forward<K>(k));
    }

};

template<typename T>
using AtLeast = AtLeastImpl< std::decay_t<T> >;

//------------------------------------------------------------------------------
template<typename T>
struct EqualToImpl
{
    const T value;

    template<typename K>
    bool test (K&& k) const
    { return ( value == std::forward<K>(k) ); }

    template<typename K>
    bool setup (K&& k) const
    { return test(std::forward<K>(k)); }

};

template<typename T>
using EqualTo = EqualToImpl< std::decay_t<T> >;

//------------------------------------------------------------------------------
template<typename T>
struct GreaterThanImpl
{
    const T value;

    template<typename K>
    bool test (K&& k) const
    { return ( value < std::forward<K>(k) ); }

    template<typename K>
    bool setup (K&& k) const
    { return test(std::forward<K>(k)); }

};

template<typename T>
using GreaterThan = GreaterThanImpl< std::decay_t<T> >;

//******************************************************************************
}  // namespace checker
//******************************************************************************
template<typename T>
using WaiterForChange = Waiter< T, checker::HasChanged<T> >;

template<typename T>
using WaiterForAtLeast = Waiter< T, checker::AtLeast<T> >;

template<typename T>
inline checker::EqualTo<T> isEqualTo(T&& t)
{ return checker::EqualTo<T>{std::forward<T>(t)}; }

template<typename T>
using WaiterForEqual = Waiter< T, checker::EqualTo<T> >;

template<typename T>
inline checker::GreaterThan<T> isGreaterThan(T&& t)
{ return checker::GreaterThan<T>{std::forward<T>(t)}; }

template<typename T>
using WaiterForGreater = Waiter< T, checker::GreaterThan<T> >;

//------------------------------------------------------------------------------
/**
 * @param value initial value
 * @return  a WaitObject that notifies on a change of the value
 */
template<typename T>
inline WaiterForChange<T> createWaiterForChange(T&& value)
{ return WaiterForChange<T>(std::forward<T>(value)); }

/**
 * @param value initial value
 * @return  a WaitObject that wait until at least the value provided by
 *          the wait call is reached.
 */
template<typename T>
inline WaiterForAtLeast<T> createWaiterForAtLeast(T&& value)
{ return WaiterForAtLeast<T>(std::forward<T>(value)); }

//------------------------------------------------------------------------------
/**
 * this class implements a thread safe waiter for zero
 * it is basically a template implementation of a latch
 */
template<typename T>
class WaiterForZero : public asynchronous::WaiterForEqual<T>
{
public:
    using base_t = asynchronous::WaiterForEqual<T>;
    using value_t = typename base_t::value_t;
    using lock_t = typename base_t::lock_t;
    using clock_t = typename base_t::clock_t;
    using timepoint_t = typename base_t::timepoint_t;
    using duration_t = typename base_t::duration_t;

protected:
    static void dec(size_t& v) { --v; }

    /**
     * check if the target is already reached and throw if so
     * you want to use it before modify the value
     * @return lock
     */
    lock_t check()
    {
        auto l = base_t::getLock();
        if (base_t::locked_try_wait(l))
        { throw std::logic_error("Waiter: value already zero"); }
        return l;
    }

    bool locked_count_down(const lock_t& l) { return base_t::locked_modify(l, dec); }

public:
    explicit WaiterForZero(value_t value) :
            base_t{std::move(value), asynchronous::isEqualTo<value_t>(0)}
    {}

    bool count_down() { return locked_count_down(check()); }

    bool count_down_and_wait_until(const timepoint_t& timepoint)
    {
        auto l = check();
        return base_t::locked_modify_and_wait_until(l, timepoint, dec);
    }

    bool count_down_and_wait_for(const duration_t& duration)
    { return this->count_down_and_wait_until(clock_t::now() + duration); }

    void count_down_and_wait()
    { this->count_down_and_wait_until(base_t::getInfinity()); }
};

//******************************************************************************
template<typename T, typename P, typename K>
inline WaiterImpl<T,P>& operator += (WaiterImpl<T,P>& w, K&& k)
{
    w.modify([&k](T& v) { v+=k; });
    return w;
}

template<typename T, typename P, typename K>
inline WaiterImpl<T,P>& operator -= (WaiterImpl<T,P>& w, K&& k)
{
    w.modify([&k](T& v) { v-=k; });
    return w;
}

//******************************************************************************
} // end of namespace asynchronous
//******************************************************************************
