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
// Created on: Jul 4, 2018
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include "asynchronous/doneflag.hpp"
#include <future>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * This class combines a promise and the connected future into one object
 * to ensure the life time of both.
 * It implements the promise API directly and the future API via operator ->
 */
template<typename T, typename PROMISE = typename std::promise<T> >
class FutureValue
{
public:
    using Result_t  = T;
    using Promise_t = PROMISE;
    using Future_t  = std::future<Result_t>;

private:
    Promise_t   ivPromise;
    Future_t    ivFuture;

public:
    FutureValue() :
        ivPromise(),
        ivFuture(ivPromise.get_future())
    {}

    /**
     * provides the full future API through a "smart-pointer" operator
     */
    Future_t* operator ->() { return &ivFuture; }

    /**
     * provide the promise API
     */
    template<typename... Args>
    void set_value(Args&&... args)
    { ivPromise.set_value(std::forward<Args>(args)...); }

    template<typename EXCEPTION>
    void set_exception(EXCEPTION&& exc)
    {
        using exc_t = std::decay_t<EXCEPTION>;
        if constexpr (std::is_same_v<exc_t, std::exception_ptr>)
        { ivPromise.set_exception(std::forward<EXCEPTION>(exc)); }
        else
        { ivPromise.set_exception(std::make_exception_ptr(std::forward<EXCEPTION>(exc))); }
    }
};

//------------------------------------------------------------------------------
/**
 * implements the std::promise API, but allows to call set_value/exception
 * multiples times, but only the first will actually set the value/exception
 */
template<typename T>
class OneTimePromise
{
public:
    using Result_t = T;
    using Promise_t = std::promise<T>;
private:
    asynchronous::DoneFlag ivFlag;
    Promise_t ivPromise;

    bool done() { return ivFlag.set(); }

public:
    /**
     * provide the promise API
     */

    std::future<Result_t> get_future()
    { return ivPromise.get_future(); }

    template<typename... Args>
    void set_value(Args&&... args)
    {
        if (done()) return;
        ivPromise.set_value(std::forward<Args>(args)...);
    }

    void set_exception(std::exception_ptr p)
    {
        if (done()) return;
        ivPromise.set_exception(std::move(p));
    }

};

/**
 * implements a FutureValue that can be set multiple times
 * But only the first set is acknowledged
 */
template<typename T>
using OneTimeFutureValue = asynchronous::FutureValue< T, asynchronous::OneTimePromise<T> >;

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
