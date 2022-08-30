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
// Created on: Dec 6, 2019
//     Author: oelsnerc
//******************************************************************************

#include "asynchronous/lazy_thread_pool.hpp"
#include <algorithm>

//******************************************************************************
namespace {
//******************************************************************************
template<typename T>
struct ValueResult
{
    using value_t = T;
    bool    isValid;
    value_t ivValue;

    explicit ValueResult() : isValid{false}, ivValue() {}
    explicit ValueResult(value_t&& v) : isValid(true), ivValue(std::move(v)) {}

    operator bool () const { return isValid; }
    value_t* operator -> () { return &ivValue; }
    value_t& operator * () { return ivValue; }
};

/**
* Removes the first element of the queue and return it
* if the queue is empty, the result is false
* @return either [true, nextElement] or [false,empty]
*/
template<typename T>
static ValueResult<T> try_pop(const asynchronous::LazyThreadPool::Lock&, std::queue<T>& queue)
{
    if (queue.empty()) { return ValueResult<T>{}; }
    ValueResult<T> result{std::move(queue.front())};
    queue.pop();
    return result;
}

/**
 * find the current thread in the thread list
 * @param threads
 * @return non const iterator pointing either to the thread or end
 */
using Threads = asynchronous::LazyThreadPool::Threads;
static Threads::iterator findThisThread( /*const*/ Threads& threads)
{   // we need a non const_iterator, so this has to be non-const
    const auto id = std::this_thread::get_id();
    return std::find_if(threads.begin(), threads.end(),
       [&id] (const std::thread& thread)
       { return (thread.get_id() == id); }
    );
}

/**
 * tries to find the current thread in the thread list
 * and if found, removes it from the thread list and returns it
 * @param threads
 * @return either [true, thisThread] or [false,empty]
 */
using ThreadResult = ValueResult<asynchronous::LazyThreadPool::Thread>;
static ThreadResult popThisThread(asynchronous::LazyThreadPool::Threads& threads)
{
    auto pos = findThisThread(threads);
    if (pos == threads.end()) { return ThreadResult{}; }

    ThreadResult result{ std::move(*pos) };
    threads.erase(pos);
    return result;
}

//******************************************************************************
}  // namespace anonymous
//******************************************************************************
void asynchronous::LazyThreadPool::removeThisThread()
{
    auto lck = getLock();

    auto thisThread = popThisThread(ivThreads);
    if (thisThread) { thisThread->detach(); }

    if(ivTerminating && ivThreads.empty())
    { ivTerminating->notify_all(); }
}

void asynchronous::LazyThreadPool::worker()
{
    while(auto p = try_pop(getLock(), ivQueue)) { (*p)(); }
    removeThisThread();
}

void asynchronous::LazyThreadPool::waitForThreads()
{
    auto lck = getLock();

    // this function is called only inside the destructor
    // so the owner is not allowed to call "addJob" anymore
    // that is, we don't need to set ivThreads
    // because the pool is already empty and no new jobs will be added.
    if (ivThreads.empty()) { return; }

    ivTerminating.reset( new std::condition_variable() );

    while(not ivThreads.empty())
    { ivTerminating->wait(lck); }
}

bool asynchronous::LazyThreadPool::addJob(Job&& job)
{
    auto lck = getLock();

    // the owner of this object has to ensure,
    // like for every other C++ object,
    // that no function of an object is called, while it is destroyed
    // or even already destroyed
    // if (ivTerminating) { return false; }

    ivQueue.push(std::move(job));

    if (ivThreads.size() < ivMaxNumberOfThreads)
    {
        ivThreads.emplace_back( std::thread{&LazyThreadPool::worker, this} );
    }

    return true;
}

