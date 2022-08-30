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
#include "asynchronous/waiter.hpp"

//******************************************************************************
namespace asynchronous {
//******************************************************************************
/**
 * Latches are a thread co-ordination mechanism that allow one or more threads
 * to block until an operation is completed.
 * An individual latch is a single-use object; once the operation has been completed,
 * it cannot be re-used.
 * see http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3666.html
 */
using latch = asynchronous::WaiterForZero<size_t>;

//******************************************************************************
} // end of namespace asynchronous
//******************************************************************************
