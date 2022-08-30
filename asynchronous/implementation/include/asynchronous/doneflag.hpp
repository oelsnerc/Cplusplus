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
// Created on: Jun 29, 2018
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include <memory>
#include <atomic>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * implement a flag that can set only once thread-safely
 */
class DoneFlag
{
private:
    using value_t = std::atomic_flag;
    std::unique_ptr<value_t> ivFlag;    // atomic_flag is not movable

public:
    DoneFlag() : ivFlag(std::make_unique<value_t>(false)) {};

    bool set() { return ivFlag->test_and_set(); }
};

//------------------------------------------------------------------------------
/**
 * implement a flag that can be set and reset thread-safely
 */
class Flag
{
private:
    using value_t = std::atomic_bool;
    std::unique_ptr<value_t> ivFlag;    // atomic_flag is not movable

public:
    explicit Flag(bool value = false) : ivFlag(std::make_unique<value_t>(value)) {};

    bool set(bool value = true) { return ivFlag->exchange(value); }
    operator bool () const { return ivFlag->load(); }
};

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
