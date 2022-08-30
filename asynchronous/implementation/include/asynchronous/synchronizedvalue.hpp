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
#include <mutex>

//******************************************************************************
namespace asynchronous {
//******************************************************************************
/**
 * Provide a "guard" object, that holds the lock while existing
 * Through this guard one can access the value in the "usual" way
 * it helps to see this as a "pointer" to value
 */
template<typename ValueType>
class Updater
{
public:
    using value_t = ValueType;
    using mutex_t = std::mutex;
    using guard_t = std::unique_lock<mutex_t>;   // lock is movable, guard is not

private:
    guard_t     ivLock;
    value_t*    ivValuePtr;

    template<typename T>
    friend class SynchronizedValue;

    /**
     * Create the guard object to enable easy synchronized access
     * but create it only by this SynchronizedValue
     * @param MutexRef the mutex to be locked while we exist
     * @param ValuePtr the value we want to grant access to
     */
    Updater(mutex_t& MutexRef, value_t* ValuePtr) :
        ivLock(MutexRef),
        ivValuePtr(ValuePtr)
    {}

public:
    ~Updater() = default;
    Updater(const Updater& other) = delete;
    Updater& operator = (const Updater& other) = delete;

    Updater(Updater&& other) :
        ivLock(std::move(other.ivLock)),
        ivValuePtr(other.ivValuePtr)
    {
        other.ivValuePtr = NULL;
    }

    Updater& operator = (Updater&& other)
    {
        if (&other != this)
        {
            ivLock = std::move(other.ivLock);
            ivValuePtr = other.ivValuePtr;
            other.ivValuePtr = NULL;
        }
        return *this;
    }

    /**
     * cast operator to return a copy of the value
     */
    operator value_t () const { return *ivValuePtr; }

    /**
     * pointer access operator to return a pointer to the value
     * this enables the chaining of "->" to let the compiler
     * collapse all "->" into only one.
     * @return pointer to the value
     */
    value_t* operator -> () { return ivValuePtr; }
    const value_t* operator -> () const { return ivValuePtr; }

    /**
     * pointer dereference operator to a return a ref to the value
     * @return reference to the value
     */
    value_t& operator * () { return *ivValuePtr; }
    const value_t& operator * () const { return *ivValuePtr; }

    /**
     * Enable the direct assignment of the value
     * @param other can be of any type that ValueType implements "=" for
     * @return reference to this updater (to enable chaining)
     */
    template<typename OtherType>
    Updater& operator = (const OtherType& other) { *ivValuePtr = other; return *this; }

    /**
     * Enable direct compare to the value
     * @param other can be of any type that ValueType implements "==" for
     * @return whatever "==" returns converted to bool
     */
    template<typename OtherType>
    bool operator == (const OtherType& other) const { return (*ivValuePtr == other); }
};

//------------------------------------------------------------------------------
/**
 * This class provides a simple to use thread-safe access to a value
 */
template<typename ValueType>
class SynchronizedValue
{
public:
    using Update_t = Updater<ValueType>;
    using ConstUpdate_t = Updater< const ValueType>;
    using value_t  = typename Update_t::value_t;
    using mutex_t  = typename Update_t::mutex_t;

private:
    value_t         ivValue;
    mutable mutex_t ivMutex;

public:
    /**
     * creates the value with these parameters
     * @param args the parameters to the constructor of the value
     */
    template<typename... Args>
    explicit SynchronizedValue(Args&&... args) :
        ivValue(std::forward<Args>(args)...)
    { }

    ~SynchronizedValue() = default;
    SynchronizedValue(const SynchronizedValue& other) = delete;
    SynchronizedValue& operator = (const SynchronizedValue& other) = delete;

    /**
     * Create an update object that holds the lock while existing.
     * @return an guarding object providing synchronized access to the value
     */
    Update_t getUpdater() { return Update_t(ivMutex, &ivValue); }
    ConstUpdate_t getUpdater() const { return ConstUpdate_t(ivMutex, &ivValue); }

    /**
     * dereference operator to return a guarding updater object
     * @return an guarding object providing synchronized access to the value
     */
    Update_t operator* () { return Update_t(ivMutex, &ivValue); }
    ConstUpdate_t operator* () const { return ConstUpdate_t(ivMutex, &ivValue); }

    /**
     * access operator to return a guarding updater object
     * @return an guarding object providing synchronized access to the value
     */
    Update_t operator -> () { return Update_t(ivMutex, &ivValue); }
    ConstUpdate_t operator -> () const { return ConstUpdate_t(ivMutex, &ivValue); }
};

//******************************************************************************
}   // end of namespace asynchronous
//******************************************************************************
