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
// Created on: Apr 26, 2022
//     Author: oelsnerc
//******************************************************************************
#include "TermParser/keyvaluenode.hpp"
#include "TermParser/termparser.hpp"

#include <exception>
#include <sstream>

//******************************************************************************
namespace {
//------------------------------------------------------------------------------
using Queue = mmc::TermNode::Queue;
using Priority = mmc::TermNode::Priority;
using CreatorFunc = mmc::TermNode::ptr();

template<typename ... ARGS>
[[noreturn]] static void throwError(ARGS&&... args)
{
    std::ostringstream s;
    s << "Error: ";
    ( s << ...  << std::forward<ARGS>(args) );
    throw std::invalid_argument{ s.str() };
}

static auto pop_front(Queue& queue)
{
    auto result = std::move(queue.front());
    queue.pop_front();
    return result;
}

static auto pop_back(Queue& queue)
{
    mmc::TermNode::ptr result;
    if ( not queue.empty() )
    {
        result = std::move(queue.back());
        queue.pop_back();
    }
    return result;
}

static auto pop_preBack(Queue& queue)
{
    auto result = pop_back(queue);
    result.swap(queue.back());
    return result;
}

//------------------------------------------------------------------------------
using Variables = mmc::KeyValueNode<char, mmc::TermNode::value_t>;

static Variables& getVariables()
{
    static Variables variables;
    return variables;
}

//------------------------------------------------------------------------------
template<typename T, typename ... ARGS>
static mmc::TermNode::ptr create(ARGS&&... args)
{ return std::make_unique<T>(std::forward<ARGS&&>(args)...); }

static Queue parse(const char* (&begin), const char *end, Priority priority = Priority::none);

static const char * getBinaryParams(const char *begin, const char *end, Queue& queue, Priority priority,
                                    mmc::TermNode::ptr& left,
                                    mmc::TermNode::ptr& right)
{
    if (queue.size() < 2) { throwError("missing left side operand"); }

    auto rightSide = ::parse(begin, end, priority);
    if (rightSide.empty()) { throwError("missing right side operand!"); }
    if (rightSide.size() > 1) { throwError("stray term found!"); }

    left  = pop_preBack(queue);
    right = pop_front(rightSide);
    return begin;
}

template<size_t PARAMNUMBER>
static Queue getParams(const char* (&begin), const char* end, Priority priority)
{
    Queue result = ::parse(begin, end, priority);

    const auto size = result.size();
    if (size < 1) { throwError("Missing function parameter!"); }

    if constexpr (PARAMNUMBER > 0)
    {
        if (size < PARAMNUMBER) { throwError("Missing function parameter!"); }
        if (size > PARAMNUMBER) { throwError("More than ", PARAMNUMBER, " parameters found!"); }
    }
    return result;
}

//------------------------------------------------------------------------------
struct Number : mmc::TermNode
{
    value_t ivValue;

    explicit Number(value_t value = value_t{} ) : ivValue(std::move(value)) {}

    [[nodiscard]] Priority getPriority() const override { return Priority::number; }

    const char * parse(const char *begin, const char *end, Queue &/*queue*/) override
    {
        value_t  val = 0;
        auto* pos = begin;
        while(pos != end )
        {
            auto& c = *pos;
            if (c == '\0') { break; }
            if (c < '0') { break; }
            if (c > '9') { break; }
            val = val*10 + (c-'0');
            ++pos;
        }
        if (pos == begin) // nothing converted
        { throwError("Could not convert \"", begin, "\" into a number!"); }

        ivValue = std::move(val);
        return pos;
    }

    [[nodiscard]] value_t calc() const override { return ivValue; }

    std::ostream& printTo(std::ostream& ostream) const override
    {
        ostream << ivValue;
        return ostream;
    }
};

struct Variable : mmc::TermNode
{
    std::string ivName;

    explicit Variable(std::string name) :
        ivName(std::move(name))
    {

    }

    [[nodiscard]] Priority getPriority() const override { return Priority::number; }

    const char * parse(const char *begin, const char */*end*/, Queue &/*queue*/) override
    { return begin; }

    [[nodiscard]] value_t calc() const override { return mmc::getVariable(ivName); }

    std::ostream& printTo(std::ostream& ostream) const override
    {
        ostream << ivName;
        return ostream;
    }

};

//------------------------------------------------------------------------------
struct TwoOperands : mmc::TermNode
{
    mmc::TermNode::ptr ivLeft;
    mmc::TermNode::ptr ivRight;

    explicit TwoOperands() = default;

    explicit TwoOperands(mmc::TermNode::ptr&& leftSide, mmc::TermNode::ptr&& rightSide) :
        ivLeft(std::move(leftSide)),
        ivRight(std::move(rightSide))
    { }

    const char * parse(const char *begin, const char *end, Queue& queue) override
    { return getBinaryParams(begin, end, queue, getPriority(), ivLeft, ivRight); }

    virtual void printName(std::ostream&) const = 0;

    std::ostream& printTo(std::ostream& ostream) const override
    {
        ostream << '(';
        ivLeft->printTo(ostream);
        printName(ostream);
        ivRight->printTo(ostream);
        ostream << ')';
        return ostream;
    }
};

struct Addition : TwoOperands
{
    [[nodiscard]] Priority getPriority() const override { return Priority::add; }
    void printName(std::ostream & ostream) const override { ostream << '+'; }

    const char * parse(const char *begin, const char *end, Queue& queue) override
    {
        if (queue.size() < 2)
        {
            auto ptr = create<Number>(0);
            ptr.swap(queue.back());
            queue.push_back( std::move(ptr) );
        }
        return TwoOperands::parse(begin, end, queue);
    }

    [[nodiscard]] value_t calc() const override
    { return ivLeft->calc() + ivRight->calc(); }
};

struct Subtraction : Addition
{
    void printName(std::ostream & ostream) const override { ostream << '-'; }

    [[nodiscard]] value_t calc() const override
    { return ivLeft->calc() - ivRight->calc(); }
};

struct Multiplication : TwoOperands
{
    using TwoOperands::TwoOperands;

    [[nodiscard]] Priority getPriority() const override { return Priority::mul; }
    void printName(std::ostream & ostream) const override { ostream << '*'; }


    [[nodiscard]] value_t calc() const override
    { return ivLeft->calc() * ivRight->calc(); }

};

struct Division : Multiplication
{
    void printName(std::ostream & ostream) const override { ostream << '/'; }

    [[nodiscard]] value_t calc() const override
    {
        auto left = ivLeft->calc();
        auto right = ivRight->calc();
        if (right == 0) { throwError("division by zero!"); }
        return left / right;
    }
};

//------------------------------------------------------------------------------
struct Comma : mmc::TermNode
{
    [[nodiscard]] Priority getPriority() const override { return Priority::comma; }

    const char* parse(const char* /*begin*/, const char* /*end*/, Queue& /*queue*/) override
    { throwError("Stray parameter!"); }

    [[nodiscard]] value_t calc() const override
    { throwError("Comma should never be evaluated"); }

    std::ostream & printTo(std::ostream &) const override
    { throwError("Comma should never be printed"); }
};

struct Parenthesis_Close : mmc::TermNode
{
    [[nodiscard]] Priority getPriority() const override { return Priority::parenthesis_close; }

    const char* parse(const char* /*begin*/, const char* /*end*/, Queue& /*queue*/) override
    { throwError("Missing Opening '('!"); }

    [[nodiscard]] value_t calc() const override
    { throwError("Closing Parenthesis should never be evaluated"); }

    std::ostream & printTo(std::ostream &) const override
    { throwError("Closing Parenthesis should never be printed"); }
};

struct Parenthesis_Open : mmc::TermNode
{
    [[nodiscard]] Priority getPriority() const override { return Priority::parenthesis_open; }

    const char* parse(const char* begin, const char* end, Queue& queue) override
    {
        auto me = pop_back(queue);
        auto leftSide = pop_back(queue);

        auto pos = begin;
        while(true)
        {
            for (auto& termPtr : ::parse(pos, end, Priority::none))
            { queue.emplace_back( std::move(termPtr)); }

            if (pos == end) { throwError("Closing ')' missing!"); }
            if (*pos == ',') { ++pos; continue; }

            if (*pos != ')') { throwError("Missing closing ')'!"); }
            break;
        }

        if ( leftSide && (queue.size() == 1) )
        {
            auto rightSide = pop_back(queue);
            queue.emplace_back( create<Multiplication>(std::move(leftSide), std::move(rightSide)));
        }

        return ++pos;
    }

    [[nodiscard]] value_t calc() const override
    { throwError("Opening Parenthesis should never be evaluated"); }

    std::ostream & printTo(std::ostream &) const override
    { throwError("Opening Parenthesis should never be printed"); }
};

//------------------------------------------------------------------------------
static void evaluateImplicitMultiplication(Queue& queue)
{
    if (queue.size() < 2) { return; }

    auto rightSide = pop_back(queue);
    auto leftSide = pop_back(queue);
    queue.emplace_back( create<Multiplication>(std::move(leftSide), std::move(rightSide)));
}

template<size_t PARAMNUMBER>
struct Function : mmc::TermNode
{
    Queue ivParams;

    [[nodiscard]] Priority getPriority() const override { return Priority::function; }

    const char* getParams(const char* pos, const char* end, Queue& /*queue*/, Priority priority)
    {
        ivParams = ::getParams<PARAMNUMBER>(pos, end, priority);
        return pos;
    }

    const char* parse(const char* begin, const char* end, Queue& queue) override
    {
        auto pos = getParams(begin, end, queue, getPriority());
        evaluateImplicitMultiplication(queue);
        return pos;
    }

    [[nodiscard]] auto& getTerm(size_t index) const
    { return ivParams.at(index); }

    virtual void printName(std::ostream & ) const = 0;
    std::ostream & printTo(std::ostream & ostream) const override
    {
        printName(ostream);
        ostream << '(';
        for(size_t pos = 0; pos < ivParams.size()-1; ++pos)
        {
            getTerm(pos)->printTo(ostream);
            ostream << ',';
        }
        ivParams.back()->printTo(ostream);
        ostream << ')';
        return ostream;
    }
};

//------------------------------------------------------------------------------
struct Square : Function<1>
{
    void printName(std::ostream & ostream) const override { ostream << "sqr"; }
    [[nodiscard]] value_t calc() const override
    {
        auto val = getTerm(0)->calc();
        return val*val;
    }
};

struct Faculty_Func : Function<1>
{
    void printName(std::ostream & ostream) const override { ostream << "fac"; }

    [[nodiscard]] value_t calc() const override
    {
        value_t result = getTerm(0)->calc();
        for(auto v = result-1; v>1 ; --v)
        { result *= v; }
        return result;
    }
};

struct Faculty_Operand : Faculty_Func
{
    const char * parse(const char *begin, const char */*end*/, Queue& queue) override
    {
        if (queue.size() < 2) { throwError("missing left side operand"); }
        ivParams.emplace_back(pop_preBack(queue));
        return begin;
    }
};

struct Power_Func : Function<2>
{
    void printName(std::ostream & ostream) const override { ostream << "pow"; }

    [[nodiscard]] value_t calc() const override
    {
        auto base = getTerm(0)->calc();
        auto pow = getTerm(1)->calc();
        value_t result = 1;
        for(;pow > 0; --pow) { result *= base; }
        return result;
    }
};

struct Power_Operator : Power_Func
{
    const char* parse(const char* begin, const char* end, Queue& queue) override
    {
        auto& left = ivParams.emplace_back();
        auto& right = ivParams.emplace_back();
        return getBinaryParams(begin, end, queue, getPriority(), left, right);
    }
};

struct Average : Function<0>
{
    void printName(std::ostream & ostream) const override { ostream << "avg"; }

    [[nodiscard]] value_t calc() const override
    {
        value_t result = 0;
        if (ivParams.empty()) { return result; }
        for (const auto& item: ivParams) { result += item->calc(); }
        result /= static_cast<value_t>(ivParams.size());
        return result;
    }
};

//------------------------------------------------------------------------------
static auto getCreators()
{
    mmc::KeyValueNode<char, CreatorFunc*> creators;

    creators.emplace("+", create<Addition>);
    creators.emplace("-", create<Subtraction>);
    creators.emplace("*", create<Multiplication>);
    creators.emplace("/", create<Division>);
    creators.emplace(",", create<Comma>);
    creators.emplace("(", create<Parenthesis_Open>);
    creators.emplace(")", create<Parenthesis_Close>);
    creators.emplace("!", create<Faculty_Operand>);
    creators.emplace("^", create<Power_Operator>);
    creators.emplace("sqr", create<Square>);
    creators.emplace("pow", create<Power_Func>);
    creators.emplace("fac", create<Faculty_Func>);
    creators.emplace("avg", create<Average>);
    return creators;
}

static auto getNextStatement(const char* (&begin), const char *end)
{
    static const auto creators = getCreators();
    auto [ pos, creator_ptr ] = creators.find(begin, end);

    mmc::TermNode::ptr result;
    if (creator_ptr) { result = (*creator_ptr) (); }
    else { result = create<Number>(); }

    begin = pos;
    return result;
}


static Queue parse(const char* (&begin), const char *end, Priority priority)
{
    auto pos = begin;

    Queue result;
    while(pos < end)
    {
        auto nextPos = pos;
        auto termPtr = getNextStatement(nextPos, end);
        if (termPtr->getPriority() <= priority) { break; }
        result.push_back(std::move(termPtr));
        pos = result.back()->parse(nextPos,end, result);
    }

    begin = pos;
    return result;
}

//------------------------------------------------------------------------------
} // end of anonymous namespace
//------------------------------------------------------------------------------
mmc::Term::Term(const char* begin, const char* end) :
    ivRoot()
{
    auto pos = begin;
    auto queue = ::parse(pos, end);
    if (queue.empty()) { throwError("Empty term!"); }
    if (queue.size() != 1) { throwError('\"', begin, "\" multiple terms found!"); }
    if (pos != end) { throwError('\"', begin, "\" could not convert everything to a term! \"", pos, '\"'); }
    ivRoot = pop_front(queue);
}

void mmc::setVariable(const char* name, mmc::TermNode::value_t value)
{
    getVariables().emplace(name, std::move(value));
}

mmc::TermNode::value_t& mmc::getVariable(const char* name)
{
    auto [_, value_ptr] = getVariables().find(name);
    if (value_ptr) { return const_cast<mmc::TermNode::value_t&>(*value_ptr); }
    throwError("No variable with name \"", name, "\"!");
}

//******************************************************************************
// EOF
//******************************************************************************