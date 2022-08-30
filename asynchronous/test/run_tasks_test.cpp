/*
    IBM Confidential
    Licensed Internal Code Source Materials

    3931, 3932 Licensed Internal Code

    (C) Copyright IBM Corp. 2018, 2022

    The source code for this program is not published or otherwise
    divested of its trade secrets, irrespective of what has
    been deposited with the U.S. Copyright Office.
*/

#include "asynchronous/run_tasks.hpp"
#include <mutex>
#include <iostream>
#include <thread>
#include <map>
#include <gtest/gtest.h>


std::atomic_int count(0);

class run_tasks_test : public ::testing::Test {
public:
    using Map = std::map< std::thread::id, int>;
    using Mutex = std::mutex;
    using ULock = std::unique_lock<Mutex>;
private:
    mutable Mutex ivMutex;
    Map   ivMap;
    ULock getLock() const { return ULock(ivMutex); }
public:

    using ::testing::Test::Test;

    void tick() {
        auto lck = getLock();
        auto id = std::this_thread::get_id();
        auto iter = ivMap.find(id);
        if(iter == ivMap.end()) {
            ivMap.emplace_hint(iter, id, 1);
        } else {
            auto & count = iter->second;
            ++count;
        }
    }

    size_t getThreads() const {
        auto lck = getLock();
        return ivMap.size();
    }

    asynchronous::Tasks makeTasks(size_t count) {
        asynchronous::Tasks tasks;
        tasks.reserve(count);
        for(size_t i=0; i<count; ++i) {
            tasks.emplace_back([this] () {this->tick();});
        }
        return tasks;
    }

    std::ostream & toStream(std::ostream & s) const {
        auto lck = getLock();
        for(auto & p : ivMap) {
            s << "tid:" << std::hex << p.first << ", invoked:" << std::dec << p.second << '\n';
        }
        return s;
    }

};


std::ostream & operator<<(std::ostream & s, const run_tasks_test & t) {
    return t.toStream(s);
}

TEST_F(run_tasks_test, moreTasksThanThreads) {

    constexpr size_t maxTasks   = 120;
    constexpr size_t maxThreads =   4;

    auto tasks = makeTasks(maxTasks);

    EXPECT_EQ(0u, getThreads());
    {
        EXPECT_NO_THROW( asynchronous::run_tasks(tasks,maxThreads) );
    }
    EXPECT_GE(maxThreads, getThreads());

    SCOPED_TRACE(*this);
}

TEST_F(run_tasks_test, moreThreadsThanTasks) {

    constexpr size_t maxTasks   =   4;
    constexpr size_t maxThreads = 120;

    auto tasks = makeTasks(maxTasks);

    EXPECT_EQ(0u, getThreads());
    {
        EXPECT_NO_THROW( asynchronous::run_tasks(tasks,maxThreads) );
    }
    EXPECT_GE(maxTasks, getThreads());

    SCOPED_TRACE(*this);

}


TEST_F(run_tasks_test, tasksButNoThreads) {

    constexpr size_t maxTasks   = 4;
    constexpr size_t maxThreads = 0;
    auto tasks = makeTasks(maxTasks);

    EXPECT_THROW( asynchronous::run_tasks(tasks,maxThreads), std::invalid_argument);

    SCOPED_TRACE(*this);

}


TEST_F(run_tasks_test, noTasksNoThreads) {

    constexpr size_t maxTasks   = 0;
    constexpr size_t maxThreads = 0;

    auto tasks = makeTasks(maxTasks);

    EXPECT_NO_THROW(asynchronous::run_tasks(tasks,maxThreads));
    SCOPED_TRACE(*this);

}

TEST_F(run_tasks_test, noTasksButThreads) {

    constexpr size_t maxTasks   = 0;
    constexpr size_t maxThreads = 4;

    auto tasks = makeTasks(maxTasks);

    EXPECT_NO_THROW(asynchronous::run_tasks(tasks,maxThreads));

    SCOPED_TRACE(*this);

}
