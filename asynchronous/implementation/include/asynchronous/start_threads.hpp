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
// Created on: Feb 27, 2019
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include "asynchronous/futurevalue.hpp"

#include <functional>
#include <future>
#include <vector>
#include <mutex>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

namespace details {
//******************************************************************************

template<typename FUNC, typename... ARGS>
struct result_t
{
    using type = typename std::result_of< FUNC(ARGS...) >::type;
    using future = typename std::future<type>;
    using container = typename std::vector<future>;
};

//------------------------------------------------------------------------------
/**
 * this class can be used to walk thread-safe through an iterator range
 */
template<typename ITERATOR>
class IteratorWalker
{
public:
    using iterator = ITERATOR;
    using pointer =  typename std::iterator_traits<iterator>::pointer;
    using mutex_t = std::mutex;
    using lock_t = std::unique_lock<mutex_t>;

private:
    iterator ivPos;
    iterator ivEnd;
    mutex_t  ivMutex;

    lock_t getLock() { return lock_t(ivMutex); }

public:
    explicit IteratorWalker(iterator begin, iterator end) :
            ivPos(std::move(begin)),
            ivEnd(std::move(end))
    {}

    pointer getNext()
    {
        auto lck = getLock();

        if (ivPos == ivEnd) { return nullptr; }

        auto result = &(*ivPos);
        ++ivPos;
        return result;
    }
};

/**
 * @brief Functor to work on each iteration
 * @tparam ITERATOR
 */
template<typename ITERATOR>
struct Walker
{
    using iterator = IteratorWalker<ITERATOR>;

    /**
     * @brief iterate over walker and call func with args on each iteration
     * @param walker
     * @param func
     * @param args
     */
    template<typename FUNC, typename... ARGS>
    void operator () (iterator& walker, FUNC&& func, ARGS&&... args) const
    {
        while(auto* e = walker.getNext())
        { std::invoke(func, args..., *e); }
    }
};

template<typename CONTAINER>
inline size_t getSize(const CONTAINER& container)
{
    using std::begin;
    using std::end;
    return static_cast<size_t>( std::distance(begin(container), end(container)) );
}

//------------------------------------------------------------------------------
/**
 * @brief This class is basically a threadpool working on a container
 *        it runs the function func for each element in the container
 *        in threadCount threads
 * @tparam RESULT the return type of the function
 * @tparam CONTAINER container type
 */
template<typename RESULT, typename CONTAINER>
class ValueThreads
{
private:
    using values_t = CONTAINER;
    using value_iterator = decltype( std::begin( std::declval<values_t>() ));

    using result_t = std::decay_t<RESULT>;
    using results_t = std::vector< asynchronous::FutureValue<result_t> >;
    using result_iterator = typename results_t::iterator;

    struct Iterators
    {
        value_iterator  ivValueIterator;
        value_iterator  ivValueEnd;
        result_iterator ivResultIterator;

        explicit Iterators(values_t values,
                           results_t& results) :
            ivValueIterator( std::begin(values) ),
            ivValueEnd( std::end(values) ),
            ivResultIterator( std::begin(results) )
        { }

        bool next()
        {
            if (ivValueIterator == ivValueEnd) { return false; }
            ++ivValueIterator;
            ++ivResultIterator;
            return true;
        }
    };

    static size_t getSize(size_t threadCount, const values_t& values)
    {
        if (threadCount == 0) { return 0; }
        return details::getSize(values);
    }

    struct SharedData
    {
        std::mutex  ivMutex;
        results_t   ivResults;
        Iterators   ivIterators;

        explicit SharedData(size_t threadCount, values_t values) :
                ivMutex(),
                ivResults(getSize(threadCount, values) ),
                ivIterators(values, ivResults)
        {}

        bool getIterators(value_iterator &valueIterator, result_iterator &resultIterator)
        {
            std::unique_lock lock{ivMutex};
            valueIterator = ivIterators.ivValueIterator;
            resultIterator = ivIterators.ivResultIterator;
            return ivIterators.next();
        };
    };

    /**
     * @brief The actual worker function
     *        loops over all elements in the container and calls the function as
     *        result = func(element, args...);
     * @param sharedData reference to the results and the iterators
     * @param func  function to be call on each element
     * @param args  extra parameters to the function
     */
    template<typename FUNC, typename ... ARGS>
    static void run(SharedData& sharedData, FUNC&& func, ARGS&&... args)
    {
        value_iterator valueIterator;
        result_iterator resultIterator;

        while(sharedData.getIterators(valueIterator, resultIterator))
        {
            try
            {
                if constexpr( std::is_void_v<RESULT> )
                {
                    std::invoke(func, *valueIterator, args...);
                    resultIterator->set_value( );
                }
                else
                { resultIterator->set_value( std::invoke(func, *valueIterator, args...) ); }
            }
            catch(...)
            { resultIterator->set_exception( std::current_exception()); }
        }
    }

    struct Threads : public std::vector< std::thread>
    {
         ~Threads()
         {
             for(auto& thread : *this)
             { thread.join(); }
         }
    };

    /**
     * @brief having the SharedData in a unique Ptr makes this whole struct
     *        not copy-able but movable
     *        The "run" function needs only a reference to the actual SharedData object
     *        which should stay valid until all "run" returned.
     *        NOTE: The threads will join, before the ivShared is destroyed!
     */
    std::unique_ptr<SharedData> ivSharedDataPtr;
    Threads ivThreads;

public:
    /**
     * @brief initialize all threads to set the results
     *        with calling func on each element in values
     * @param threadCount number of threads to run on
     * @param values any container that supports std::begin, end and the member function size
     * @param func to be call on every element in values
     * @param args extra parameters to func
     */
    template<typename FUNC, typename ... ARGS>
    explicit ValueThreads(size_t threadCount, values_t values, FUNC&& func, ARGS&&... args) :
            ivSharedDataPtr( new SharedData{threadCount, values} ),
            ivThreads()
    {
        threadCount = std::min(threadCount, details::getSize(values));
        ivThreads.reserve( threadCount );
        for(size_t i = 0; i < threadCount; ++i)
        {
            try
            {
                ivThreads.emplace_back(&ValueThreads::run<FUNC, ARGS...>,
                                     std::ref(*ivSharedDataPtr),
                                     func,
                                     std::forward<ARGS>(args)...);
            } catch(std::system_error&)
            { /*ignore if we can not start the thread*/}
        }
    }

    auto begin() const { return std::begin(ivSharedDataPtr->ivResults); }
    auto end() const { return std::end(ivSharedDataPtr->ivResults); }

    auto empty() const { return ivSharedDataPtr->ivResults.empty(); }
    auto size() const { return ivSharedDataPtr->ivResults.size(); }

    auto threadCount() const { return ivThreads.size(); }
};

//******************************************************************************
}  // namespace details

/**
 * invoke the function func in another thread and return a future to its result.
 * @param func callable
 * @param args parameters
 * @return future to the result of func
 */
template <typename FUNC, typename... ARGS, typename RESULT = typename details::result_t<FUNC, ARGS...> >
inline typename RESULT::future invoke_async(FUNC&& func, ARGS&&... args)
{
    return std::async(std::launch::async, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
}

/**
 * invoke the member function on the object_ptr t in another thread
 * and return a future to its result
 * @param memfunc member function of the struct T
 * @param object_ptr to the struct T
 * @return future to the result of memfunc
 */
template <typename T, typename FUNC, typename... ARGS, typename RESULT = typename details::result_t<FUNC T::*, T*, ARGS...>>
inline typename RESULT::future invoke_async(FUNC T::* memfunc, T* obj_ptr, ARGS&&... args)
{
    return asynchronous::invoke_async(std::mem_fn(memfunc), obj_ptr, std::forward< ARGS >(args)...);
}

/**
 * start threadCount threads and invoke func in each of them
 * and return a vector of futures for all results
 * @param threadCount
 * @param func
 * @return vector of future to the results
 */
template<typename FUNC, typename... ARGS, typename RESULT = typename details::result_t<FUNC&&, ARGS&&...> >
inline typename RESULT::container invoke_threads(size_t threadCount, FUNC&& func, ARGS&&... args)
{
    typename RESULT::container result;
    result.reserve(threadCount);

    for(size_t i = 0; i < threadCount; ++i)
    { result.emplace_back( invoke_async( func, args...) ); }

    return result;
}

/**
 * run the function in threadCount threads and return after all have returned
 * if threadCount is zero it returns immediately
 * NOTE: "func" should not throw any exception, because there is no way
 *       to handle it and the threads might terminate before completion.
 *       And since the calling thread is a worker too, it is undefined
 *       which exception gets propagated.
 * @param threadCount number of threads to use (incl. the calling thread)
 * @param func
 * @param args
 */
template<typename FUNC, typename... ARGS >
inline void run_threads(size_t threadCount, FUNC&& func, ARGS&&... args)
{
    if (threadCount == 0) { return; }

    auto threads = invoke_threads( threadCount-1, std::forward<FUNC>(func), std::forward<ARGS>(args)...);
    std::invoke( std::forward<FUNC>(func), std::forward<ARGS>(args)...);

    // the destructors of all futures in "threads" wait for their threads to join
}

//------------------------------------------------------------------------------

/**
 * call func for each element in the container in threadCount threads
 * NOTE: func should not throw, for the same reasons as in "run_threads"
 * @param threadCount number of threads to use (incl. the calling thread)
 * @param container holding the elements
 * @param func will be called as func(args..., element&)
 * @param args function parameter preceding the element
 */
template<typename CONTAINER , typename FUNC, typename... ARGS>
inline void for_each(size_t threadCount, CONTAINER&& container, FUNC&& func, ARGS&&... args)
{
    using ITERATOR = decltype( std::begin(container) );
    details::IteratorWalker<ITERATOR> iter(std::begin(container), std::end(container));

    run_threads(threadCount,
                details::Walker<ITERATOR>{},
                std::ref(iter),
                std::forward<FUNC>(func),
                std::forward<ARGS>(args)...);
}

//------------------------------------------------------------------------------
/**
 * @brief invokes the callable func on each element in container
 *        in parallel on at most threadCount threads, but
 *        no more threads than elements in the container
 *        NOTE: the number of threads used depend on the available resources
 *              but will be at most threadCount
 *
 * @example:
 *     constexpr size_t threadCount = 2;
 *     std::vector<int> numbers = { 1,2,3,4,5 };
 *     auto results = asynchronous::invoke_on_each(
 *                      threadCount,        // use at most threadCount threads
 *                      numbers,            // work on this container
 *                      [](int &a, int b)   // the first parameter is the element, all others are the extra parameters
 *                      {                   // a can be a non-const ref, because the container is non-const
 *                           auto t = a;
 *                           a += b;        // we're the only thread working on this particular element!
 *                           return t;      // return type is int
 *                      }, 1);              // the extra parameter (in this case mapped onto b)
 *
 *     EXPECT_EQ(numbers.size(), results.size());
 *     EXPECT_GE(threadCount, results.threadCount());  // threadCount >= actual number of threads, because systems have a maximum available resource count
 *     EXPECT_LT(0u, results.threadCount());           // but at least one thread should be started!
 *
 *     int sum = 0;
 *     for(auto& result : results)
 *     { sum += result->get(); }            // check the futures
 *
 *     EXPECT_EQ( 15, sum);                 // all futures returned!
 *     EXPECT_EQ( (std::vector<int>{2,3,4,5,6}), numbers );
 *
 * @param threadCount number of threads to be used
 *        NOTE: using 0 threads will not invoke the func at all
 * @param container any container supporting std::begin/end
 * @param func will be invoked on each element in the container, like
 *        result = func(element, args...);
 * @param args extra parameters to the func
 * @return an object representing all results
 */
template<typename CONTAINER , typename FUNC, typename... ARGS,
        typename VALUEITERATOR = decltype( std::begin( std::declval<CONTAINER>()) ) >
inline auto invoke_on_each(size_t threadCount, CONTAINER&& container, FUNC&& func, ARGS&&... args)
{
    using VALUE = decltype( *(std::declval<VALUEITERATOR>()) );
    using FUNC_RESULT = typename details::result_t<FUNC, VALUE, ARGS...>::type;
    using VTHREADS = details::ValueThreads<FUNC_RESULT, CONTAINER>;

    return VTHREADS {threadCount,
                     std::forward<CONTAINER>(container),
                     std::forward<FUNC>(func),
                     std::forward<ARGS>(args)...};
}

/**
 * @brief invokes the callable func on each element in the container in
 *        at most number of elements count of threads
 * @param container any container supporting std::begin/end
 * @param func will be invoked on each element in the container, like
 *        result = func(element, args...);
 * @param args extra parameters to the func
 * @return an object representing all results
 */
template<typename CONTAINER , typename FUNC, typename... ARGS,
        typename = decltype( std::begin( std::declval<CONTAINER>()) ) >
inline auto invoke_on_each(CONTAINER&& container, FUNC&& func, ARGS&&... args)
{
    const auto threadCount = details::getSize(container);
    return invoke_on_each(threadCount,
                          std::forward<CONTAINER>(container),
                          std::forward<FUNC>(func),
                          std::forward<ARGS>(args)...);
}

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
