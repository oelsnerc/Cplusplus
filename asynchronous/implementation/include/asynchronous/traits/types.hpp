/*
    IBM Confidential
   Licensed Internal Code Source Materials

    3931, 3932 Licensed Internal Code

    (C) Copyright IBM Corp. 2018, 2022

    The source code for this program is not published or otherwise
    divested of its trade secrets, irrespective of what has
    been deposited with the U.S. Copyright Office. */

#pragma once

#include <vector>
#include <future>

namespace asynchronous { namespace traits {

    struct task_t {
        using result_t  = void;
        using type      = std::packaged_task< result_t() >;
        using future_t  = std::future<result_t>;
    };

    using container_t   = std::vector<task_t::type>;


}}
