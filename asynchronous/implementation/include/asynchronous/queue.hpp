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
// Created on: Sep 12, 2018
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include "asynchronous/cappedqueue.hpp"

#include <functional>
#include <limits>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * This implements a thread-safe FIFO queue
 * The managed type has to be default constructable.
 * The function "pop" is blocking ("try_pop" is not).
 */
template<typename T>
using Queue = asynchronous::CappedQueue<T, std::numeric_limits<size_t>::max()>;

/**
 * create a Queue that contains the pointers to the elements in the range [begin, end)
 * NOTE: as the nature of pointers is, they are only valid as long as the iterators are valid.
 * @param begin iterator pointing to the range begin
 * @param end range end
 * @return Queue< element* >
 */
template<typename ITERATOR, typename T = typename std::remove_reference< decltype( *(ITERATOR{}) )>::type >
inline Queue<T*> createPtrQueue(ITERATOR begin, const ITERATOR& end)
{
    Queue<T*> queue;
    auto l = queue->getLock();

    for(; begin != end; ++begin)
    {
        queue->push_no_notify(l, &(*begin));
    }

    return queue;
}

/**
 * create a Queue of pointers to the elements in the container
 * @param container std::begin/end have to be defined
 * @return Queue< element* >
 */
template<typename CONTAINER>
inline auto createPtrQueue(CONTAINER&& container) ->
    decltype( createPtrQueue(std::begin(container), std::end(container)))
{ return createPtrQueue(std::begin(container), std::end(container)); }

/**
 * calls func with the args and the current element
 * in the queue as long as the queue is not empty
 * @param queue Queue of pointers to the elements
 * @param func to be called
 * @param args function parameters preceding the element
 */
template<typename T, typename FUNC, typename... ARGS>
inline void workOnPtrQueue( asynchronous::Queue< T* >& queue, FUNC&& func, ARGS&&... args)
{
    while(auto r = queue->try_pop())
    { std::invoke(func, args..., *(r.value)); }
}

//------------------------------------------------------------------------------
/**
 * implement a Reader/Writer interface for the capped queue
 * When the last writer is destroyed, all pop's return with State::empty
 */
template<typename T>
using SharedQueue = asynchronous::SharedCappedQueue<T, std::numeric_limits<size_t>::max()>;

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
