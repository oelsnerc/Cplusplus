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
// Created on: Dec 5, 2019
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include <functional>
#include <thread>
#include <queue>
#include <list>
#include <mutex>
#include <condition_variable>
#include <memory>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * this class implements a threadpool that allocates threads only
 * and as long as there are jobs to do.
 * NOTE: - the destructor waits for all jobs to finish, which were
 *         scheduled prior to the destructor
 *         Calling addJob while the destructor is running is undefined behavior
 *       - this class is not moveable because of its mutex
 */
class LazyThreadPool
{
public:
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

    using ConditionPtr = std::unique_ptr< std::condition_variable >;

    using Job = std::function<void(void)>;
    using Queue = std::queue<Job>;

    using Thread = std::thread;
    using Threads = std::list<Thread>;

private:
    mutable Mutex   ivMutex;
    const size_t    ivMaxNumberOfThreads;
    ConditionPtr    ivTerminating;
    Queue           ivQueue;
    Threads         ivThreads;

    Lock getLock() const { return Lock{ivMutex}; }

    /**
     * tries to find this thread in the thread list and
     * if it finds it, removes it from this list.
     * NOTE: since the thread object is destroyed right after leaving
     *       the scope of this function, we have to detach the thread
     *       that means, after this function the thread should end
     *       immediately
     */
    void removeThisThread();

    /**
     * a simple worker that runs in a thread as long as jobs are in the queue
     * and remove the thread from the thread list after that
     */
    void worker();

    /**
     * return only if the thread list is empty
     */
    void waitForThreads();

public:
    explicit LazyThreadPool(size_t maxNumberOfThreads) :
        ivMutex(),
        ivMaxNumberOfThreads(maxNumberOfThreads),
        ivTerminating(),
        ivQueue(),
        ivThreads()
    {}

    /**
     * the mutex prohibits copy and move
     * so let's make it explicit
     */
    LazyThreadPool(const LazyThreadPool&) = delete;
    LazyThreadPool& operator = (const LazyThreadPool&) = delete;
    LazyThreadPool(LazyThreadPool&&) = delete;
    LazyThreadPool& operator = (LazyThreadPool&&) = delete;

    /**
     * This blocks until the thread list is empty
     */
    ~LazyThreadPool()
    {
        waitForThreads();
    }

    /**
     * tries to add this job to the queue
     * NOTE: if you call this while the object is being destroyed
     *       this is undefined behavior
     * @param job
     * @return true if the job was added to the queue
     */
    bool addJob(Job&& job);

    /**
     * see the other overload
     * @param func first parameter to std::bind
     * @param args the following parameters to std::bind
     * @return true if the job was added to the queue
     */
    template<typename FUNC, typename... ARGS>
    bool addJob(FUNC&& func, ARGS&&... args)
    { return addJob( Job(std::bind(std::forward<FUNC>(func), std::forward<ARGS>(args)...))); }
};

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
