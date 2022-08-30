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
// Created on: Sep 17, 2018
//     Author: oelsnerc
//******************************************************************************

#pragma once

//******************************************************************************
#include <memory>

//******************************************************************************
namespace asynchronous {
//******************************************************************************

/**
 * This implements a Reader/Writer of a resource interface.
 * When the last writer disappears, the states function "notifyToFinish" is called.
 */

/**
 * this is the reader
 * this represents only a shared resource
 * You don't want to create a reader by itself.
 * You want to create a writer and get a reader from it
 * That way we can notify the readers when the last writer died.
 */
template<typename STATE>
class Reader
{
public:
    using state_t = STATE;
    using value_t = typename state_t::value_t;
    using result_t = typename state_t::result_t;

protected:
    std::shared_ptr<state_t> ivState;

public:
    explicit Reader() :
        Reader(std::make_shared<state_t>())
    {}

    explicit Reader(std::shared_ptr<state_t> state) :
        ivState(std::move(state))
    {}

    const state_t* operator->() const { return ivState.operator->(); }
    state_t* operator->() { return ivState.operator->(); }
};

//------------------------------------------------------------------------------
/**
 * this is the writer
 * when last writer of the same state is destroyed,
 * the state's function "notifyToFinish" is called.
 * Which can be used to notify all readers to return.
 */
template<typename STATE>
class Writer : public Reader<STATE>
{
public:
    using reader_t = Reader<STATE>;
    using state_t = typename reader_t::state_t;
    using value_t = typename reader_t::value_t;
    using result_t = typename reader_t::result_t;

private:
    struct DoneSetter
    {
        state_t* state;
        DoneSetter(state_t* s) : state(s) {}
        ~DoneSetter() { state->notifyToFinish(); }
    };

    std::shared_ptr<DoneSetter> ivDone;

public:
    Writer() :
        Writer(std::make_shared<state_t>())
    {}

    explicit Writer(std::shared_ptr<state_t> state) :
        reader_t(std::move(state)),
        ivDone(std::make_shared<DoneSetter>(this->ivState.get()))
    {}

    reader_t as_reader() const { return *this; }
    Writer as_writer() const { return *this; }
};

//******************************************************************************
}  // namespace asynchronous
//******************************************************************************
