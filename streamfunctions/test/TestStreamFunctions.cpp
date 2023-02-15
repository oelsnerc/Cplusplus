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
#include "streamfunctions/StreamFunctions.hpp"

#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <functional>
#include <chrono>

//------------------------------------------------------------------------------
#include <gtest/gtest.h>

#define EXPECT_EQ_FUNC( String, f) {    \
    std::stringstream OStream;              \
    f;                                      \
    EXPECT_EQ(String, OStream.str());       \
}

//******************************************************************************
TEST(StreamFunctions, BasicConversions)
{
    const char* Greetings = "Hello World!";

    std::stringstream ptr_str;
    ptr_str << static_cast<const void*>(Greetings);

    EXPECT_EQ(ptr_str.str(), StreamFunc::toString(StreamFunc::ptr(Greetings)));

    EXPECT_EQ(Greetings, StreamFunc::toString("Hello", ' ', "World", '!'));

    EXPECT_EQ("true", StreamFunc::b2a(42 > 21));
}

//------------------------------------------------------------------------------
namespace mytypes {
//------------------------------------------------------------------------------
struct PrintToMember
{
    int value;

    template<typename STREAM>
    void printTo(STREAM& stream) const
    {
        stream << value;
    }
};

struct MyValue
{
    int value;

    explicit MyValue(int v) : value(v) {}

    template<typename STREAM>
    void printTo(STREAM& stream) const
    {
        stream << value;
    }
};

// NOTE: the print function has to be found by ADL
//       i.e. in either namespace of the stream or the value
template<typename STREAM>
static void print(STREAM& stream, const mytypes::MyValue& value)
{
    stream << value.value+1;
}

struct StreamOperator
{
    int value;

    template<typename STREAM>
    friend STREAM& operator << (STREAM& stream, const StreamOperator& object)
    {
        stream << object.value;
        return stream;
    }
};

struct CallOperator
{
    int value;

    template<typename STREAM>
    STREAM& operator () (STREAM& stream) const
    {
        stream << value;
        return stream;
    }
};

//------------------------------------------------------------------------------
} // end of namespace mytypes
//------------------------------------------------------------------------------

TEST(StreamFunctions, BasicPrinterObject)
{
    EXPECT_EQ("42", StreamFunc::Printer{mytypes::PrintToMember{42}});   // use the printTo member
    EXPECT_EQ("43", StreamFunc::Printer{mytypes::MyValue{42}});         // the print overload is prefered over member printTo
    EXPECT_EQ("44", StreamFunc::Printer{mytypes::StreamOperator{44}});  // use the operator << overload
    EXPECT_EQ("45", StreamFunc::Printer{mytypes::CallOperator{45}});    // use the callable operator
    EXPECT_EQ("46", StreamFunc::Printer{[](auto& stream){ stream << 46; }}); // use the callable operator

    EXPECT_EQ("47", StreamFunc::identity(47));
    EXPECT_EQ("48", StreamFunc::identity("48"));
}

//------------------------------------------------------------------------------
TEST(StreamFunctions, IntegerConversion)
{
    EXPECT_EQ_FUNC("042", StreamFunc::print_unsigned<10>(OStream, 42u, 3));
    EXPECT_EQ_FUNC("042", (StreamFunc::print_unsigned<10,'a'>(OStream, 42u, 3)) );

    EXPECT_EQ_FUNC("02A", StreamFunc::print_unsigned_base(OStream, 42u, 16u, 3));
    EXPECT_EQ_FUNC("02a", StreamFunc::print_unsigned_base<'a'>(OStream, 42u, 16u, 3));
}

//------------------------------------------------------------------------------
TEST(StreamFunctions, BasicHexDump)
{
    static const char* Greetings = "Hello World!";
    static const std::string strGreetings(Greetings);
    static const char* Greetings_HexDump = "0000: 48 65 6C 6C 6F 20 57 6F 72 6C 64 21";

    EXPECT_EQ_FUNC(Greetings_HexDump, StreamFunc::dump_hexadecimal<4>(OStream, Greetings, Greetings+strlen(Greetings)));

    EXPECT_EQ_FUNC(Greetings_HexDump, StreamFunc::dump_hexadecimal<4>(OStream, strGreetings));
    EXPECT_EQ_FUNC(Greetings_HexDump, StreamFunc::dump_hexadecimal<4>(OStream, Greetings));

    static const size_t memory_size = 6;
    static const int memory[memory_size] = {1,2,3,4,5,6};

    EXPECT_EQ_FUNC("", StreamFunc::dump_hexadecimal<4>(OStream, memory, 0));

    static const char* memory_HexDump_littleEndian = "0000: 01 00 00 00 02 00 00 00 03 00 00 00 04 00 00 00\n" \
                                                     "0010: 05 00 00 00 06 00 00 00";
    static const char* memory_HexDump_bigEndian    = "0000: 00 00 00 01 00 00 00 02 00 00 00 03 00 00 00 04\n" \
                                                     "0010: 00 00 00 05 00 00 00 06";

    auto* bytePtr = reinterpret_cast<const uint8_t*>(memory);
    auto* memory_HexDump = (*bytePtr == 0x00) ? memory_HexDump_bigEndian : memory_HexDump_littleEndian;
    EXPECT_EQ_FUNC(memory_HexDump, StreamFunc::dump_hexadecimal<4>(OStream, memory, memory_size));

    // EXPECT_EQ_FUNC(StreamFunc::dump_hexadecimal<4>(OStream, &strGreetings, 1), "0000: 14 F2 24 08");
}

//------------------------------------------------------------------------------
TEST(StreamFunctions, BasicBinDump)
{
    static const char* Greetings = "Hello World!";
    static const std::string strGreetings(Greetings);
    static const char* Greetings_BinDump = "0000: 01001000 01100101 01101100 01101100\n" \
                                           "0004: 01101111 00100000 01010111 01101111\n" \
                                           "0008: 01110010 01101100 01100100 00100001\n";

    EXPECT_EQ_FUNC(Greetings_BinDump, StreamFunc::dump_binary<4>(OStream, Greetings, Greetings+strlen(Greetings)));

    EXPECT_EQ_FUNC(Greetings_BinDump, StreamFunc::dump_binary<4>(OStream, strGreetings));
    EXPECT_EQ_FUNC(Greetings_BinDump, StreamFunc::dump_binary<4>(OStream, Greetings));

    static const size_t memory_size = 7;
    static const uint8_t memory[memory_size] = {1,2,3,4,5,255,8};
    static const char* memory_BinDump = "0000: 00000001 00000010 00000011 00000100\n" \
                                        "0004: 00000101 11111111 00001000";

    EXPECT_EQ_FUNC(memory_BinDump, StreamFunc::dump_binary<4>(OStream, memory, memory_size));
    EXPECT_EQ_FUNC("", StreamFunc::dump_binary<4>(OStream, memory, 0));
}

//******************************************************************************
TEST(StreamFunctions, BeginEnd)
{
    static const std::vector<int> MyVector = { 1,2,3,4 };
    EXPECT_EQ(MyVector.begin(), StreamFunc::begin(MyVector));
    EXPECT_EQ(MyVector.end(), StreamFunc::end(MyVector));

    static const int MyArray[] = { 1,2,3,4 };
    EXPECT_EQ(&MyArray[0], StreamFunc::begin(MyArray));
    EXPECT_EQ(&MyArray[4], StreamFunc::end(MyArray));

    static const char* MyStrings[] =  { "eins", "zwei", "drei" };
    EXPECT_EQ(&MyStrings[0], StreamFunc::begin(MyStrings));
    EXPECT_EQ(&MyStrings[3], StreamFunc::end(MyStrings));

    EXPECT_EQ(5u, StreamFunc::strlen("Hello"));
    EXPECT_EQ(5u, StreamFunc::strlen(std::string("Hello")));
    EXPECT_EQ(5u, StreamFunc::strlen(std::string_view("Hello")));
}

TEST(StreamFunctions, PrintSequence_String)
{
    static const char* Greetings = "Hello World!";
    static const std::string strGreetings(Greetings);

    EXPECT_EQ_FUNC("H-e-l-l-o- -W-o-r-l-d-!",
            StreamFunc::print_sequence(OStream, Greetings, Greetings+strlen(Greetings), '-'));

    EXPECT_EQ_FUNC("H-e-l-l-o- -W-o-r-l-d-!",
            StreamFunc::print_sequence(OStream, Greetings, '-'));

    EXPECT_EQ_FUNC("H...e...l...l...o... ...W...o...r...l...d...!",
            StreamFunc::print_sequence(OStream, Greetings, Greetings+strlen(Greetings), StreamFunc::identity("...") ));

    EXPECT_EQ_FUNC("H___e___l___l___o___ ___W___o___r___l___d___!",
            StreamFunc::print_sequence(OStream, strGreetings, "___"));
}

TEST(StreamFunctions, PrintSequence_Array)
{
    static const int MyValues [] = { 1, 2, 3, 4 };

    EXPECT_EQ_FUNC("1,2,3,4",
            StreamFunc::print_sequence(OStream, &MyValues[0], &MyValues[4], ','));

    EXPECT_EQ_FUNC("1,2,3,4",
            StreamFunc::print_sequence(OStream, MyValues, ','));
}

//------------------------------------------------------------------------------
// Output:
// 1,2,3,4
// 0001,0002,0003,0004
// -1--,-2--,-3--,-4--

TEST(StreamFunctions, BasicPrinter)
{
    static const std::vector<int> MyValues = { 1,2,3,4 };

    EXPECT_EQ_FUNC("1,2,3,4",
            StreamFunc::print_sequence_as(OStream, MyValues.begin(),MyValues.end(), ',', StreamFunc::printer::identity()));

    EXPECT_EQ_FUNC("0001,0002,0003,0004",
            StreamFunc::print_sequence_as(OStream, MyValues.begin(),MyValues.end(), ',', StreamFunc::printer::hex(4)));

    EXPECT_EQ_FUNC("-1--,-2--,-3--,-4--",
            StreamFunc::print_sequence_as(OStream, MyValues.begin(),MyValues.end(), ',', StreamFunc::printer::Align::Center(4,'-')));

    EXPECT_EQ_FUNC("-1--,-2--,-3--,-4--",
            StreamFunc::print_sequence_as(OStream, MyValues, ',', StreamFunc::printer::Align::Center(4,'-')));
}

//------------------------------------------------------------------------------
// Output:
// 3. 1
// 4. 2
// 5. 3
// 6. 4
// a. 1
// b. 2
// c. 3
// d. 4
// A. 1
// B. 2
// C. 3
// D. 4
TEST(StreamFunctions, AdvancedPrinter)
{
    using StreamFunc::printer::enumeration;

    static const std::vector<int> MyValues = { 1,2,3,4 };
    static const std::vector<char> MyItems = { 'a','b','c','d' };
    const char* MyStrings = "ABCD";

    EXPECT_EQ_FUNC("3. 1\n4. 2\n5. 3\n6. 4",
            StreamFunc::print_sequence_as(OStream, MyValues.begin(),MyValues.end(), '\n', enumeration(3,". ")));

    EXPECT_EQ_FUNC("a. 1\nb. 2\nc. 3\nd. 4",
            StreamFunc::print_sequence_as(OStream, MyValues.begin(),MyValues.end(), '\n', enumeration(StreamFunc::printer::iterator(MyItems.begin()),". ")));

    EXPECT_EQ_FUNC("A. 1\nB. 2\nC. 3\nD. 4",
            StreamFunc::print_sequence_as(OStream, MyValues.begin(),MyValues.end(), '\n', enumeration(StreamFunc::printer::iterator(MyStrings),". ")));
}

//------------------------------------------------------------------------------
// test the normal, copy and move construction of a printer
// the trace of "PrinterLifeTime" should show something like
// 0xffe6f6df->tracer
// 0xffe6f6b5->tracer move
// 0xffe6f58f->tracer copy
// 0xffe6f58f  operator() 21
// 0xffe6f58f  operator() 32
// 0xffe6f58f  operator() 43
// 0xffe6f58f  operator() 54
// 0xffe6f58f  operator() 65
// 0xffe6f58f<-~tracer
// 0xffe6f6b5<-~tracer
// 0xffe6f6df<-~tracer

//struct tracer
//{
//    tracer() { FENTRYM(); }
//    ~tracer() { FEXITM(); };
//
//    tracer(const tracer&) { FENTRYM("copy"); }
//    tracer(tracer&&) { FENTRYM("move"); }
//
//    template<typename StreamType, typename ObjectType>
//    StreamType& operator () (StreamType& OStream, const ObjectType& object) const
//    {
//        FTRACEM(object);
//        OStream << object;
//        return OStream;
//    }
//};
//
//TEST(StreamFunctions, PrinterLifeTime)
//{
//    std::vector<int> MyVec{21,32,43,54,65};
//
//    using namespace StreamFunc;
//
//    EXPECT_EQ(seq_as(MyVec, tracer()),"21,32,43,54,65");
//}

//------------------------------------------------------------------------------
TEST(StreamFunctions, PrinterRefs)
{
    std::vector<int> MyVec{21,32,43,54,65};

    using namespace StreamFunc;

    auto p = printer::enumeration(0,'-');

    EXPECT_EQ("0-21\n1-32\n2-43\n3-54\n4-65",
            seq_as(MyVec, std::ref(p),'\n'));

    EXPECT_EQ("5-21\n6-32\n7-43\n8-54\n9-65",
            seq_as(MyVec, std::ref(p),'\n'));
}

//******************************************************************************
TEST(StreamFunctions, BasicWrapper)
{
    EXPECT_EQ("002A", StreamFunc::hex(42,4));
    EXPECT_EQ("042", StreamFunc::dec(42,3));
    EXPECT_EQ("101010", StreamFunc::bin(42));
}

//------------------------------------------------------------------------------
TEST(StreamFunctions, SeqWrapperVector)
{
    static const std::vector<int> MyValues = { 9,10,11,12 };

    auto seq = StreamFunc::seq(MyValues);
    EXPECT_EQ("9,10,11,12", seq);
    EXPECT_EQ("0009,000A,000B,000C", StreamFunc::seq_hex(MyValues,4));
    EXPECT_EQ("00001001,00001010,00001011,00001100", StreamFunc::seq_bin(MyValues,8));
}

TEST(StreamFunctions, SeqWrapperArray)
{
    static int MyValues[] = { 9, 10, 11, 12 };

    EXPECT_EQ("9,10,11,12", StreamFunc::seq(MyValues));
    EXPECT_EQ("9:10:11", StreamFunc::seq(MyValues, MyValues+3, ':'));
    EXPECT_EQ("0009,000A,000B,000C", StreamFunc::seq_hex(MyValues,4));
    EXPECT_EQ("00001001,00001010,00001011,00001100", StreamFunc::seq_bin(MyValues,8));

    // 'seq_hex(const uint8_t* const&, const uint8_t*)
    const int* const MyValuesPtr = MyValues;
    EXPECT_EQ("9:10:11", StreamFunc::seq(MyValuesPtr, MyValuesPtr+3, ':'));
}


static void check(const char* string, int argc, const char* argv[])
{
    EXPECT_EQ(string, StreamFunc::seq(argv, argv+argc, ','));
}

TEST(StreamFunctions, SeqWrapperCString)
{
    EXPECT_EQ("A+B+C+D", StreamFunc::seq("ABCD",'+'));
    EXPECT_EQ("", StreamFunc::seq("",'+'));
    EXPECT_EQ("", StreamFunc::seq(static_cast<const char*>(nullptr),'+'));

    static const char* argv[] = { "eins", "zwei", "drei" };
    check("eins,zwei,drei", 3, argv);
}

//------------------------------------------------------------------------------
TEST(StreamFunctions, SeqEmptySep)
{
    static const unsigned char MyValues[] = { 9,10,11,12 };

    EXPECT_EQ("090A0B0C", StreamFunc::seq_hex(MyValues,MyValues+4,2, StreamFunc::empty));
}

//------------------------------------------------------------------------------
template<typename StreamType, typename ValueType>
static StreamType& just_print(StreamType& OStream, const ValueType& Value)
{
    OStream << Value;
    return OStream;
}

TEST(StreamFunctions, Seq_As_Function)
{
    static const std::vector<int> MyValues = { 9,10,11,12 };

    EXPECT_EQ("9,10,11,12", StreamFunc::seq_as(MyValues,just_print<std::ostream,int>));
}

TEST(StreamFunctions, Aligment)
{
    EXPECT_EQ("Hello           ", StreamFunc::align_left("Hello", 16));
    EXPECT_EQ("           Hello", StreamFunc::align_right("Hello", 16));
    EXPECT_EQ("     Hello      ", StreamFunc::align_center("Hello", 16));

    EXPECT_EQ("Hello-----------", StreamFunc::align_left("Hello", 16, '-'));
    EXPECT_EQ("-----------Hello", StreamFunc::align_right("Hello", 16, '-'));
    EXPECT_EQ("-----Hello------", StreamFunc::align_center("Hello", 16, '-'));

    EXPECT_EQ("Hello", StreamFunc::align_left("Hello", 4));
    EXPECT_EQ("Hello", StreamFunc::align_right("Hello", 4));
    EXPECT_EQ("Hello", StreamFunc::align_center("Hello", 4));
}


TEST(StreamFunctions, to_upper)
{
    EXPECT_EQ("HELLO", StreamFunc::to_upper("Hello"));
    EXPECT_EQ("hello", StreamFunc::to_lower("HELLO"));

    const char Hello_Array[] = "Hello";
    EXPECT_EQ("HELLO", StreamFunc::to_upper(Hello_Array));
    EXPECT_EQ("HELL", StreamFunc::to_upper(Hello_Array, Hello_Array+4));

    const std::vector<char> Hello_Vec = { 'H', 'e', 'l', 'l', 'o' };
    EXPECT_EQ("HELLO", StreamFunc::to_upper(Hello_Vec));
    EXPECT_EQ("HELL", StreamFunc::to_upper(Hello_Vec.begin(), Hello_Vec.begin()+4));
}

struct MyValue
{
    unsigned value;
};

template<unsigned char BASE, char BASE_CHAR = 'A'>
static void printAs(std::ostream& s, const MyValue& v, unsigned int digits)
{ s << "0x" << StreamFunc::hex(v.value, digits); }

TEST(StreamFunctions, hex)
{
    const int val = 257;
    EXPECT_EQ("101", StreamFunc::hex(val));
    EXPECT_EQ("2A", StreamFunc::hex(42));
    EXPECT_EQ("2a", StreamFunc::hex_lowercase(42));
    EXPECT_EQ("2A", StreamFunc::hex_uppercase(42));
    EXPECT_EQ("0x2A", StreamFunc::hex( MyValue{42} ));

    EXPECT_EQ("FFFFFFFD", StreamFunc::hex(-3));
}

TEST(StreamFunctions, hex_dump)
{
    const char* text = "Hello World!";
    auto end = text + strlen(text);

    EXPECT_EQ("0000: 48 65 6C 6C 6F 20 57 6F 72 6C 64 21", StreamFunc::hex_dump(text, end));

    const std::vector<uint8_t> numbers{ 0, 1, 2, 3, 4 ,5, 6, 7, 8, 9, 10, 11 };
    EXPECT_EQ("0000: 00 01 02 03 04 05 06 07 08 09 0A 0B", StreamFunc::hex_dump(numbers));


    std::vector<uint8_t> more_numbers;
    for(size_t i = 0; i < 42; ++i) { more_numbers.emplace_back(static_cast<uint8_t>(i)); }

    const auto expected_str = "0000: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                              "0010: 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F\n"
                              "0020: 20 21 22 23 24 25 26 27 28 29";
    EXPECT_EQ(expected_str, StreamFunc::hex_dump(more_numbers.data(), more_numbers.size()));
}

//******************************************************************************
// the trace of "BasicTrace" should show:
//  TestBody 01,12,23,34,45,56
//TEST(StreamFunctions, BasicTrace)
//{
//    const size_t Size = 6;
//    const unsigned char MyMem[Size] = { 0x1, 0x12, 0x23, 0x34, 0x45, 0x56 };
//    using namespace StreamFunc;
//
//    FTRACEF(seq_hex(MyMem,MyMem+Size, 2));
//}

//------------------------------------------------------------------------------
TEST(StreamFunctions, hex_default)
{
    const uint32_t a = 0;

    EXPECT_EQ("0", StreamFunc::hex(a));
    EXPECT_EQ("0000000000000000", StreamFunc::callPrintAs<16>(a,16));
}

//------------------------------------------------------------------------------
TEST(StreamFunctions, chrono_duration)
{
    using namespace std::chrono;

    EXPECT_EQ("2min", StreamFunc::time(minutes(2)));
    EXPECT_EQ("1h42min", StreamFunc::time(hours{1} + minutes(42)));
    EXPECT_EQ("1h42min23s", StreamFunc::time(hours{1} + minutes(42) + seconds(23)));
    EXPECT_EQ("1h42min23s65ms", StreamFunc::time(hours{1} + minutes(42) + seconds(23) + milliseconds(65)));
    EXPECT_EQ("1h65ms", StreamFunc::time(hours{1} + milliseconds(65)));
    EXPECT_EQ("12ms345us", StreamFunc::time(nanoseconds(12345678)));

    EXPECT_EQ("120min", StreamFunc::time_as<std::chrono::minutes>(hours(2)));
    EXPECT_EQ("162min", StreamFunc::time_as<std::chrono::minutes>(hours(2) + minutes(42) + seconds(23)));
    EXPECT_EQ("0min", StreamFunc::time_as<std::chrono::minutes>(hours(0)));
    EXPECT_EQ("12345678ns", StreamFunc::time_as<std::chrono::nanoseconds>(nanoseconds(12345678)));
    EXPECT_EQ("12345us", StreamFunc::time_as<std::chrono::microseconds>(nanoseconds(12345678)));

    const steady_clock::time_point christmas{ hours(438113) + minutes(12) + seconds(34) + microseconds(567890) };
    EXPECT_EQ("12-24-19 17:12:34.567890", StreamFunc::time(christmas));
    EXPECT_EQ("01-01-70 00:00:00.000000", StreamFunc::time(system_clock::time_point()));

    EXPECT_EQ(        "12.003456", StreamFunc::time_log(seconds(12) + nanoseconds(3456789)));
    EXPECT_EQ("1577207554.567890", StreamFunc::time_log(christmas - steady_clock::time_point()));

}

//******************************************************************************
// EOF
//******************************************************************************
