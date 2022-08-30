/* begin copyright

   IBM Confidential

   Licensed Internal Code Source Materials

   3931, 3932 Licensed Internal Code

   (C) Copyright IBM Corp. 2015, 2022

   The source code for this program is not published or otherwise
   divested of its trade secrets, irrespective of what has
   been deposited with the U.S. Copyright Office.

   end copyright
*/

//******************************************************************************
#include "asynchronous/synchronizedvalue.hpp"

#include <iostream>
#include <map>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

//------------------------------------------------------------------------------
#define FENTRYM(...) {}
#define FTRACEM(...) {}
#define FEXITM(...) {}

//******************************************************************************
struct Tracer
{
    int ivValue;

    Tracer() : ivValue(0) { FENTRYM(ivValue); }
    explicit Tracer(int value) : ivValue(value) { FENTRYM(ivValue); }
    Tracer(const Tracer& other) : ivValue(other.ivValue) { FENTRYM(ivValue); }
    ~Tracer()  { FEXITM(ivValue); }
    Tracer& operator = (const Tracer& other)
    {
        if ( this != &other )
        {
            FTRACEM(ivValue, ',', other.ivValue);
            ivValue = other.ivValue;
        }
        return *this;
    }

    bool operator == (const Tracer& other) const { return (ivValue == other.ivValue); }

    bool odd() const { return (ivValue & 0x01); }

    void add(int other) { ivValue += other; }

    Tracer& operator += (int other) { ivValue += other; return *this; }

};

//------------------------------------------------------------------------------
TEST(Test_SynchronizedValue, Basic)
{
    asynchronous::SynchronizedValue<Tracer>   a(42);

    ASSERT_EQ(*a, Tracer( 42 ));

    *a = Tracer( 3 );

    ASSERT_EQ(*a, Tracer( 3 ));
}

TEST(Test_SynchronizedValue, Const)
{
    static const asynchronous::SynchronizedValue<Tracer> a(43);

    ASSERT_TRUE(a->odd());

    // a->add(3);  // error: passing 'const Tracer' as 'this' argument of 'void Tracer::add(int)' discards qualifiers
}

TEST(Test_SynchronizedValue, Updater)
{
    asynchronous::SynchronizedValue<Tracer>   a(3);

    auto likeTracer = a.getUpdater();   // get the guard;

    ASSERT_EQ(likeTracer, Tracer( 3 ));

    likeTracer = Tracer( 44 );

    // likeTracer.add(3);   // error: 'class asynchronous::SynchronizedValue<Tracer>::Updater' has no member named 'add'
    // likeTracer+=3;       // error: no match for 'operator+=' in 'likeTracer += 3'

    likeTracer->add(3);         ASSERT_EQ(likeTracer, Tracer( 47 ));

    likeTracer->operator +=(3); ASSERT_EQ(likeTracer, Tracer( 50 ));
}

TEST(Test_SynchronizedValue, Map)
{
    typedef std::map<int, Tracer> map_t;
    asynchronous::SynchronizedValue<map_t> myMap;

    // (*myMap)[3] = Tracer(3); // error: no match for 'operator[]' in 'asynchronous::SynchronizedValue<ValueType>::operator*() [with ValueType = map_t]()[3]'
    (**myMap)[3] = Tracer(3);
    (*myMap)->operator [](4) = Tracer(4);

    ASSERT_EQ(myMap->size(), 2u);

    auto Map = myMap.getUpdater();

    // Map[42] = Tracer(42);    // error: no match for 'operator[]' in 'Map[42]'
    (*Map)[42] = Tracer( 42 );
    Map->operator [](43) = Tracer( 43 );

    ASSERT_EQ(Map->size(), 4u);
}

//******************************************************************************
// EOF
//******************************************************************************
