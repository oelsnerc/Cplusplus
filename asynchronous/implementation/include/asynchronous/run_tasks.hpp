/*
    IBM Confidential
   Licensed Internal Code Source Materials

    3931, 3932 Licensed Internal Code

    (C) Copyright IBM Corp. 2018, 2022

    The source code for this program is not published or otherwise
    divested of its trade secrets, irrespective of what has
    been deposited with the U.S. Copyright Office. */

#pragma once

#include "asynchronous/traits/types.hpp"

namespace asynchronous {

    using Task = traits::task_t;
    using Tasks = traits::container_t;

    /**
     * Run tasks in a limited number of threads.
     * It returns when all tasks are done

     * @param  tasks        Container of std::packaged_task.
     *                      The container must offer an index operator
     * @param  threadnumber Maximum number of threads
     * @exception std::invalid_argument if the number or threads or task are insane
     */
    void run_tasks(Tasks & tasks, size_t threadcount);

}
