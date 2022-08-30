/*
    IBM Confidential
   Licensed Internal Code Source Materials

    3931, 3932 Licensed Internal Code

    (C) Copyright IBM Corp. 2018, 2022

    The source code for this program is not published or otherwise
    divested of its trade secrets, irrespective of what has
    been deposited with the U.S. Copyright Office. */

#include <chrono>
#include <algorithm>
#include <atomic>
#include  <stdexcept>

#include "asynchronous/run_tasks.hpp"


namespace asynchronous {
    namespace {



        size_t calcNumberOfFuturesOrThrow(size_t numTask, size_t numThreads) {
            if(numTask == 0) {
                return numTask; // It's ok to do nothing
            }
            if(numThreads == 0) {
                throw std::invalid_argument("Zero number or threads is insane to execute more than zero tasks");
            }
            if(numTask + numThreads + 1 > INT32_MAX) {
                throw std::invalid_argument("Number of tasks plus threads +1 < INT32_MAX");
            }
            return numThreads;
        }

        std::vector< Task::future_t > makeFutures(size_t numTask, size_t numOfThreads) {
            std::vector< Task::future_t> threads;
            threads.reserve(calcNumberOfFuturesOrThrow(numTask, numOfThreads));
            return threads;
        }

        void run_jobs_thread(std::atomic_size_t & index_next_job,
                             Tasks & tasks)
        {
            const auto maxIndex = tasks.size();
            while (1)
            {
                const auto index = index_next_job++;
                if (index >= maxIndex)
                    break;
                auto & task = tasks[index];
                task();
            }
        }
    }

    /**
     * Run tasks in a limited number of threads.
     * It returns when all tasks are done

     * @param  tasks        container of std::packaged_task. The container must offer an index operator
     * @param  threadnumber maximum number of threads
     * @exception std::invalid_argument if the number or threads or task are insane
     */
    void run_tasks(Tasks  & tasks, size_t threadnumber) {

        std::atomic_size_t index_next_job(0);

        auto threads = makeFutures(tasks.size(), threadnumber);

        for(size_t i = 0; i < threadnumber; ++i)
        {
            threads.emplace_back(
                std::async(
                    std::launch::async,
                    run_jobs_thread,
                    std::ref(index_next_job),
                    std::ref(tasks)
                )
            );
        }
    }

}
