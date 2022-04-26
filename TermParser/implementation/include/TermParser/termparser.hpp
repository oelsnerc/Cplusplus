/* begin copyright

 IBM Confidential

 Licensed Internal Code Source Materials

 3931, 3932 Licensed Internal Code

 (C) Copyright IBM Corp. 2022, 2022

 The source code for this program is not published or otherwise
 divested of its trade secrets, irrespective of what has
 been deposited with the U.S. Copyright Office.

 end copyright
*/

//******************************************************************************
// Created on: May 02, 2022
//     Author: oelsnerc
//******************************************************************************
#pragma once

#include <memory>
#include <deque>
#include <cstring>
#include <ostream>

//******************************************************************************
namespace mmc {
//******************************************************************************

struct TermNode
{
    enum class Priority
    {
        parenthesis_close,
        comma,
        none,
        add,
        mul,
        function,
        parenthesis_open,
        number
    };

    using ptr = std::unique_ptr<TermNode>;
    using Queue = std::deque<ptr>;
    using value_t = int;

    virtual ~TermNode() = default;
    [[nodiscard]] virtual Priority getPriority() const = 0;
    virtual const char* parse(const char* begin, const char* end, Queue& queue) = 0;
    [[nodiscard]] virtual value_t calc() const = 0;
    virtual std::ostream& printTo(std::ostream&) const = 0;
};

//------------------------------------------------------------------------------

class Term
{
public:
    using value_t = TermNode::value_t;

private:
    TermNode::ptr ivRoot;

public:
    explicit Term (const char* begin, const char* end);

    explicit Term(const char* string) :
        Term(string, string + strlen(string))
    {}

    template<size_t N>
    explicit Term(const char (&string)[N]) :
        Term(string, string + N - 1)
    {}

    [[nodiscard]] value_t calc() const
    { return ivRoot->calc(); }

    std::ostream& printTo(std::ostream& ostream) const
    { return ivRoot->printTo(ostream); }

    friend std::ostream& operator << (std::ostream& ostream, const Term& term)
    { return term.printTo(ostream); }
};

//------------------------------------------------------------------------------
extern void setVariable(const char* name, mmc::TermNode::value_t value);
extern mmc::TermNode::value_t& getVariable(const char* name);
extern mmc::TermNode::value_t& getVariable(const std::string& name);

//******************************************************************************
} // end of namespace mmc
//******************************************************************************
// EOF
//******************************************************************************