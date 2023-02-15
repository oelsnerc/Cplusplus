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
#pragma once

//******************************************************************************
/*
 * This header file is meant to implement functions that are often needed on
 * output stream like objects (any object providing "<<" and "write")
 * the main idea is to keep these helpers here small and fast and use as little
 * requirements as possible to support as many stream like objects as possible
 */
//******************************************************************************
#include <type_traits>
#include <iterator>
#include <sstream>
#include <cstring>  // strlen
#include <cctype>
#include <chrono>
#include <iomanip>  // put_time

#include <iosfwd>

//******************************************************************************
namespace StreamFunc {
//******************************************************************************
namespace details {
//------------------------------------------------------------------------------

template<typename T, typename STREAM, typename = void>
struct HasMemberPrintTo : std::false_type
{ };

template<typename T, typename STREAM>
struct HasMemberPrintTo<T, STREAM, std::void_t<
        decltype ( std::declval<T>().printTo(std::declval<STREAM&>()) )
    >> : std::true_type
{ };

template<typename T, typename STREAM>
constexpr bool hasMemberPrintTo = HasMemberPrintTo<T, STREAM>::value;

//------------------------------------------------------------------------------
template<typename T, typename STREAM, typename = void>
struct HasPrintOverload : std::false_type
{ };

template<typename T, typename STREAM>
struct HasPrintOverload<T, STREAM, std::void_t<
        decltype ( print(std::declval<STREAM&>(), std::declval<T>()) )
    >> : std::true_type
{ };

template<typename T, typename STREAM>
constexpr bool hasPrintOverload = HasPrintOverload<T, STREAM>::value;

//------------------------------------------------------------------------------
template<typename T, typename STREAM, typename = void>
struct HasCallable : std::false_type
{ };

template<typename T, typename STREAM>
struct HasCallable<T, STREAM, std::void_t<
        decltype ( std::declval<T>()(std::declval<STREAM&>()) )
    >> : std::true_type
{ };

template<typename T, typename STREAM>
constexpr bool hasCallable = HasCallable<T, STREAM>::value;

//------------------------------------------------------------------------------
template<typename T, typename STREAM, typename = void>
struct HasStreamOperatorOverload : std::false_type
{ };

template<typename T, typename STREAM>
struct HasStreamOperatorOverload<T, STREAM, std::void_t<
        decltype ( std::declval<STREAM&>() << std::declval<T>() )
>> : std::true_type
{ };

template<typename T, typename STREAM>
constexpr bool hasStreamOperatorOverload = HasStreamOperatorOverload<T, STREAM>::value;

//------------------------------------------------------------------------------
} // end of namespace details
//------------------------------------------------------------------------------

/**
 * @brief the basic printer object wraps around a type
 *        that supports one of the following overloads
 *        - print(stream, object)  (a)
 *        - object.printTo(stream) (b)
 *        - object(stream)         (c)
 *        - stream << object       (d)
 *
 *        The wrapper itself provides overloads for
 *        - operator <<
 *        - implicit conversion into std::string
 *        - comparison to any type that is comparable to a string
 *
 *        NOTE: because of overload (c) the wrapped type can even be a generic lambda,
 *              which will completely optimized out, e.g.
 *              StreamFunc::Printer{[](auto& stream){ stream << 42; }}
 *
 * @tparam PRINTFUNC
 */
template<typename PRINTFUNC>
struct Printer
{
    using printer_type = std::decay_t<PRINTFUNC>;

    PRINTFUNC ivPrinter;

    constexpr explicit Printer(PRINTFUNC printer) :
        ivPrinter(std::move(printer))
    {}

    template<typename STREAM>
    void printTo(STREAM& stream) const
    {
        if constexpr( details::hasPrintOverload<printer_type, STREAM> )
        { print(stream, ivPrinter); }
        else if constexpr (details::hasMemberPrintTo<printer_type, STREAM>)
        { ivPrinter.printTo(stream); }
        else if constexpr (details::hasCallable<printer_type, STREAM>)
        { ivPrinter(stream); }
        else
        {
            static_assert(details::hasStreamOperatorOverload<printer_type, STREAM>, "is not a PrintWrapable type!");
            stream << ivPrinter;
        }
    }

    template<typename STREAM>
    friend STREAM& operator << (STREAM& stream, const Printer& value)
    {
        value.printTo(stream);
        return stream;
    }

    friend std::ostream& operator << (std::ostream& stream, const Printer& value)
    {
        value.printTo(stream);
        return stream;
    }

    std::string asSring() const
    {
        std::ostringstream s;
        printTo(s);
        return s.str();
    }

    // enable implicit conversion to string
    operator std::string() const { return asSring(); }

    // support some basic compare operators
    // we support everything that is comparable with a string
    template<typename T>
    bool operator == (const T& other) const
    { return asSring() == other; }

    template<typename T>
    bool operator != (const T& other) const
    { return not operator==(other); }

    template<typename T>
    friend bool operator == (const T& other, const Printer& printer)
    { return printer.operator==(other); }

    template<typename T>
    friend bool operator != (const T& other, const Printer& printer)
    { return printer.operator!=(other); }
};

//******************************************************************************
// Provide an object that does not print anything to the stream
// useful as a separator to print nothing between values of a sequence
constexpr Printer empty{[](auto& /*stream*/) { }};

/**
 * @brief print the object using its overloads
 *        - print(stream, object)
 *        - object.printTo(stream)
 *        - stream << object
 *        - object(stream)
 */
template<typename T>
inline auto identity(const T& object)
{
    return Printer<const T&>{object};
}

//------------------------------------------------------------------------------
/**
 * @brief print all args to that stream using the operator <<
 * @param stream
 * @param args
 * @return stream
 */
template<typename STREAM, typename... ARGS>
inline STREAM& toStream(STREAM& stream, const ARGS&... args)
{
    ( stream << ... << args);
    return stream;
}

/**
 * @param args
 * @return a string containing all args printed in that order
 */
template<typename... ARGS>
inline std::string toString(const ARGS&... args)
{
    std::ostringstream s;
    return toStream(s, args...).str();
}

//******************************************************************************
// Overloads to find the begin and end iterators for containers
// these are basically extension to include the const char* "container"
// into the std::begin and std::end
//------------------------------------------------------------------------------
using std::begin;
using std::end;

inline const char* begin(const char* string) { return string; }
inline const char* end(const char* string)
{
    auto* result = string;
    if (string) result+= ::strlen(string);
    return result;
}

/**
 * @brief provide overloads for string types
 */
using ::strlen;
inline size_t strlen(const std::string& str) { return str.size(); }
inline size_t strlen(const std::string_view& str) { return str.size(); }

//******************************************************************************
/*
 * print a bool as "true" or "false"
 * NOTE: eventhough the name is kind of cryptic, it is still following the
 *       ancient convention like "itoa", "strtoul"
 */
inline const char* b2a(bool value)
{ return (value ? "true" : "false"); }

//********************************************************************
/*
 * Explicitly print a pointer as pointer (in hex representation)
 * this is especially useful for char*
 */
template<typename PtrType>
inline const void* ptr(const PtrType* pointer)
{ return static_cast<const void*>(pointer); }

//******************************************************************************
// print number in various formats
//******************************************************************************
namespace details {
//------------------------------------------------------------------------------
/* A little Helper to make sure that a backward filled buffer has at least
 * Digits chars written.
 */

template<size_t BufferSize>
inline char* fillDigits(char (&Buffer)[BufferSize], char* Last, size_t Digits)
{
    if (Digits < 1) return Last;
    char* Begin = Buffer + BufferSize - Digits;

    if (Begin < Buffer) Begin = Buffer;
    if (Begin >= Last) return Last;

    std::fill(Begin, Last, '0');

    return Begin;
}

/**
 * @param BASE_CHAR the char representing the 10
 * @param value
 * @return the char representing this single digit value
 */
template<char BASE_CHAR, typename NumType>
inline char toChar(NumType value)
{
    if (value > 9) { return static_cast<char>(value - 10 + BASE_CHAR); }
    return static_cast<char>(value + '0');
}
//------------------------------------------------------------------------------
} // end of namespace details
//------------------------------------------------------------------------------

//******************************************************************************
// A little Helper to print an unsigned integer as its string representation
// you want to use this, if you don't know the representation base at compiletime
// NOTE: this algorithm is only for unsigned types
template<char BASE_CHAR = 'A', typename StreamType, typename NumType,
        typename std::enable_if<
                std::is_integral<NumType>::value &&
                std::is_unsigned<NumType>::value,
                int>::type = 0 >
inline StreamType& print_unsigned_base (StreamType& outputStream, NumType Value, unsigned char Base, size_t Digits = 0)
{
    if (Base < 2) return outputStream;

    //----------------------------------------------------------------
    // let's convert for a general Base
    char Str[sizeof(NumType)*8];                // calc max number of digits (i.e. binary)
    char* pos = Str + sizeof(Str);              // start at the end

    // now fill the string reversely
    do
    {
        *(--pos) = details::toChar<BASE_CHAR>(Value % Base);    // move one left and write the digit
        Value = static_cast<NumType>(Value / Base);             // calc next value
    } while (Value > 0);                                        // nothing left?

    // insert the leading zeros (if any)
    pos = details::fillDigits(Str, pos, Digits);

    // and write the string
    outputStream.write(pos,Str+sizeof(Str)-pos);
    return outputStream;
}

/* A little Helper to print an unsigned integer as its string representation
 * you want to use this, if you know the representation base at compiletime
 * so that the compiler can optimize on that
 * NOTE: this algorithm is only for unsigned types
 */
template<unsigned char BASE, char BASE_CHAR = 'A', typename StreamType, typename NumType>
inline StreamType& print_unsigned (StreamType& outputStream, NumType Value, size_t Digits=0)
{
    print_unsigned_base<BASE_CHAR>(outputStream, Value, BASE, Digits);
    return outputStream;
}

//------------------------------------------------------------------------------
// A little Helper to print a sequence as hexadecimal dump
// NOTE: this algorithm is only for unsigned types
template <size_t BASE, size_t line_width, size_t number_width, int line_number_width, typename StreamType, typename IteratorType,
        typename std::enable_if<
                std::is_integral<typename std::iterator_traits<IteratorType>::value_type>::value
//            && std::is_unsigned<typename std::iterator_traits<IteratorType>::value_type>::value
                ,int>::type = 0 >
inline StreamType& dump(StreamType& OStream, IteratorType begin, const IteratorType& end)
{
    unsigned int index = 0, line = 0;
    while (begin != end)
    {
        if (index == 0)
        {
            if (line_number_width != 0) {
                print_unsigned<16>(OStream, line*line_width, line_number_width) << ':';
            }
        }

        OStream << ' ';
        print_unsigned<BASE>(OStream, static_cast<uint8_t>(*begin++), number_width);
        ++index;

        if (index == line_width)
        {
            OStream << '\n';
            index = 0;
            ++line;
        }
    }

    return OStream;
}

//------------------------------------------------------------------------------
// A little Helper to print a sequence as hexadecimal dump
// NOTE: this algorithm is only for unsigned types
template <int line_number_width, typename StreamType, typename IteratorType,
        typename std::enable_if<
                std::is_integral<typename std::iterator_traits<IteratorType>::value_type>::value
//            && std::is_unsigned<typename std::iterator_traits<IteratorType>::value_type>::value
                ,int>::type = 0 >
inline StreamType& dump_hexadecimal(StreamType& OStream, IteratorType begin, const IteratorType& end)
{
    return dump<16, 16, 2, line_number_width>(OStream, begin, end);
}

//------------------------------------------------------------------------------
// A little Helper to print a sequence as hexadecimal dump
// NOTE: this algorithm is only for unsigned types
template <int line_number_width, typename StreamType, typename ContainerType,
        typename std::enable_if<
                std::is_integral<typename std::iterator_traits<typename ContainerType::iterator>::value_type>::value
                ,int>::type = 0 >
inline StreamType& dump_hexadecimal(StreamType& OStream, const ContainerType& Container)
{
    return dump_hexadecimal<line_number_width>(OStream, Container.begin(), Container.end());
}

//------------------------------------------------------------------------------
// A little Helper to print a memory area
template <int line_number_width, typename StreamType, typename Type>
inline StreamType& dump_hexadecimal(StreamType& OStream, const Type* begin, size_t Size)
{
    auto* bgn = reinterpret_cast<const uint8_t*>(begin);
    auto* end = reinterpret_cast<const uint8_t*>(begin + Size);
    return dump_hexadecimal<line_number_width>(OStream, bgn, end);
}
template <int line_number_width, typename StreamType, typename Type>
inline StreamType& dump_hexadecimal(StreamType& OStream, const Type* begin)
{
    return dump_hexadecimal<line_number_width>(OStream, begin, strlen(reinterpret_cast<const char*>(begin)));
}

//------------------------------------------------------------------------------
// A little Helper to print a sequence as binary dump
// NOTE: this algorithm is only for unsigned types
template <int line_number_width, typename StreamType, typename IteratorType,
        typename std::enable_if<
                std::is_integral<typename std::iterator_traits<IteratorType>::value_type>::value
//            && std::is_unsigned<typename std::iterator_traits<IteratorType>::value_type>::value
                ,int>::type = 0 >
inline StreamType& dump_binary(StreamType& OStream, IteratorType begin, const IteratorType& end)
{
    return dump<2, 4, 8, line_number_width>(OStream, begin, end);
}

//------------------------------------------------------------------------------
// A little Helper to print a sequence as hexadecimal dump
// NOTE: this algorithm is only for unsigned types
template <int line_number_width, typename StreamType, typename ContainerType,
        typename std::enable_if<
                std::is_integral<typename std::iterator_traits<typename ContainerType::iterator>::value_type>::value
                ,int>::type = 0 >
inline StreamType& dump_binary(StreamType& OStream, const ContainerType& Container)
{
    return dump_binary<line_number_width>(OStream, Container.begin(), Container.end());
}

//------------------------------------------------------------------------------
// A little Helper to print a memory area
template <int line_number_width, typename StreamType, typename Type>
inline StreamType& dump_binary(StreamType& OStream, const Type* begin, size_t Size)
{
    auto* bgn = reinterpret_cast<const uint8_t*>(begin);
    auto* end = reinterpret_cast<const uint8_t*>(begin + Size);
    return dump_binary<line_number_width>(OStream, bgn, end);
}
template <int line_number_width, typename StreamType, typename Type>
StreamType& dump_binary(StreamType& OStream, const Type* begin)
{
    return dump_binary<line_number_width>(OStream, begin, strlen(reinterpret_cast<const char*>(begin)));
}

//TODO: dump upper and lower case letters

//******************************************************************************
// functors to print an object the way we want it
//******************************************************************************
namespace printer {

//------------------------------------------------------------------------------
// just for completeness: a callable object that prints this object
struct identity
{
    template<typename StreamType, typename ObjectType>
    StreamType& operator () (StreamType& OStream, const ObjectType& object) const
    {
        OStream << StreamFunc::identity(object);
        return OStream;
    }
};

//------------------------------------------------------------------------------
// Provides a convenient way to print a number using a different representation base
// You want to use it like:
// std::cout << seq_as(MyVec, printer::number<16>(4)) << std::endl;
template<unsigned char BASE>
class number
{
private:
    unsigned int    m_Digits;
public:
    explicit number(unsigned int Digits = 0) : m_Digits(Digits) {}

    template<typename StreamType, typename NumType>
    StreamType& operator () (StreamType& OStream, NumType Value) const
    {
        typedef typename std::make_unsigned<NumType>::type UType;
        return StreamFunc::print_unsigned<BASE>(OStream, static_cast<UType>(Value), m_Digits);
    }
};

typedef number<2> bin;
typedef number<16> hex;

//------------------------------------------------------------------------------
// Alignment printers
// You want to use them as
// std::cout << seq_as(MyVec, printer::Align::Right(10), '\n') << std::endl;
//------------------------------------------------------------------------------
struct Align
{
private:
    struct Base
    {
        size_t  m_RequiredCharsNumber;
        char    m_Filler;

        explicit Base(size_t NumberOfChars, char Filler=' ') :
            m_RequiredCharsNumber(NumberOfChars),
            m_Filler(Filler)
        {}

        template<typename StreamType>
        StreamType& printFiller (StreamType& OStream, int Count) const
        {
            if (Count > 0)  OStream << std::string(static_cast<size_t>(Count), m_Filler);
            return OStream;
        }

        template<typename StreamType>
        StreamType& printNeededChars (StreamType& OStream, size_t printedChars) const
        {
            return printFiller(OStream, static_cast<int>(m_RequiredCharsNumber-printedChars));
        }
    };

public:
    class Left : protected Base
    {
    public:
        explicit Left(size_t NumberOfChars, char Filler=' ') : Base(NumberOfChars, Filler) {}

        template<typename StreamType, typename ObjectType>
        StreamType& operator () (StreamType& OStream, const ObjectType& object) const
        {
            auto object_printed = toString(object);
            OStream << object_printed;
            printNeededChars(OStream, strlen(object_printed));

            return OStream;
        }

    };

    class Right : protected Base
    {
    public:
        explicit Right(size_t NumberOfChars, char Filler=' ') : Base(NumberOfChars, Filler) {}

        template<typename StreamType, typename ObjectType>
        StreamType& operator () (StreamType& OStream, const ObjectType& object) const
        {
            auto object_printed = toString(object);
            printNeededChars(OStream, strlen(object_printed));
            OStream << object_printed;

            return OStream;
        }

    };

    class Center : protected Base
    {
    public:
        explicit Center(size_t NumberOfChars, char Filler=' ') : Base(NumberOfChars, Filler) {}

        template<typename StreamType, typename ObjectType>
        StreamType& operator () (StreamType& OStream, const ObjectType& object) const
        {
            auto object_printed = toString(object);

            int diff = static_cast<int>(m_RequiredCharsNumber - strlen(object_printed));
            int left = diff / 2;
            int right = diff - left;

            printFiller(OStream, left);
            OStream << object_printed;
            printFiller(OStream, right);

            return OStream;
        }
    };

};

//------------------------------------------------------------------------------
// wrapper to be able to use the alignment functions on normal stream operators
template<typename Allign_t, typename ObjectType>
inline auto general_align(const ObjectType& object, size_t NumberOfChars, char Filler=' ')
{
    return Printer{[&object, NumberOfChars, Filler](auto& stream)
                   {
                        Allign_t{NumberOfChars, Filler}(stream, object);
                   }};
}

//******************************************************************************
// Provide an object that prints an Enumeration Object and a Separator before
// the actual object.
// NOTE: The Enumeration Type has to provide the operator << and
//       operator ++ (int) (post increment)
//       E.g. int is such a type.
// You want to use it like:
// std::cout << seq_as(MyVec, printer::enumeration(1,"..."),'\n') << std::endl;
template<typename EnumerationType, typename SeparatorType>
class enumeration_wrapper
{
private:
    mutable EnumerationType m_Enumeration;
    SeparatorType   m_Separator;
public:
    explicit enumeration_wrapper(EnumerationType&& Start, SeparatorType&& Separator) :
        m_Enumeration(std::forward<EnumerationType>(Start)),
        m_Separator(std::forward<SeparatorType>(Separator))
    {}

    template<typename StreamType, typename ObjectType>
    StreamType& operator () (StreamType& OStream, const ObjectType& object) const
    {
        OStream << m_Enumeration++;
        OStream << m_Separator;
        OStream << object;
        return OStream;
    }
};

template<typename EnumerationType = unsigned long, typename SeparatorType=char>
inline auto enumeration( EnumerationType&& Start = 1, SeparatorType&& Separator = ' ')
{
    return enumeration_wrapper<EnumerationType,SeparatorType>(
            std::forward<EnumerationType>(Start),
            std::forward<SeparatorType>(Separator));
}

//------------------------------------------------------------------------------
// provides the wrapper around an iterator for any container
// the IteratorType has to provide the * operator and the (post)++ operator
// You want to use it like:
// std::cout << seq_as(MyVec, printer::enumeration(iterator(MyStrings.begin()), "..."),'\n') << std::endl;
template<typename IteratorType>
class iterator_wrapper
{
private:
    IteratorType    m_Pos;
public:
    explicit iterator_wrapper(IteratorType Position) : m_Pos(Position) {}

    template<typename StreamType>
    friend StreamType& operator << (StreamType& OStream, const iterator_wrapper& it)
    {
        OStream << *(it.m_Pos);
        return OStream;
    }

    iterator_wrapper<IteratorType> operator ++ (int)
    {
        return iterator_wrapper<IteratorType>(m_Pos++);
    }
};

template<typename IteratorType>
inline iterator_wrapper<IteratorType> iterator(IteratorType Position)
{
    return iterator_wrapper<IteratorType>(Position);
}


//------------------------------------------------------------------------------
// convert chars into it's upper case equivalent
struct to_upper_wrapper
{
    template<typename StreamType>
    StreamType& operator () (StreamType& OStream, char c) const
    {
        OStream << static_cast<char>(std::toupper(c));
        return OStream;
    }
};

// convert chars into it's lower case equivalent
struct to_lower_wrapper
{
    template<typename StreamType>
    StreamType& operator () (StreamType& OStream, char c) const
    {
        OStream << static_cast<char>(std::tolower(c));
        return OStream;
    }
};

//------------------------------------------------------------------------------
}   // end of namespace printer
//******************************************************************************

//------------------------------------------------------------------------------
// wrapper to enable the alignment functions to be used in normal stream operattions
// you want to use it like:
// std::cout << StreamFunc::align_center("Hello",42) << "World!";
//
template<typename ObjectType>
inline auto align_left(const ObjectType& object, size_t NumberOfChars, char Filler=' ')
{ return printer::general_align<printer::Align::Left>(object, NumberOfChars, Filler); }

template<typename ObjectType>
inline auto align_right(const ObjectType& object, size_t NumberOfChars, char Filler=' ')
{ return printer::general_align<printer::Align::Right>(object, NumberOfChars, Filler); }

template<typename ObjectType>
inline auto align_center(const ObjectType& object, size_t NumberOfChars, char Filler=' ')
{ return printer::general_align<printer::Align::Center>(object, NumberOfChars, Filler); }

//******************************************************************************
// implement a convenient way to print a number on a stream
// you can use it like:
// std::cout << StreamFunc::hex(42,4) << std::endl;
// > 002A
// >
// The "hex" function returns a Printer (functor object)
// that will print the value to the stream
// NOTE: the signed types are casted into their corresponding
//       unsigned types before printing
//******************************************************************************

//------------------------------------------------------------------------------
namespace details {
//------------------------------------------------------------------------------

template<typename T, typename PlainT = std::decay_t<T> >
constexpr bool isNumber =
        std::is_integral<PlainT>::value
        || std::is_enum<PlainT>::value;

template<typename T>
using enableIfNumber = std::enable_if_t< isNumber<T> >;

//------------------------------------------------------------------------------
} // end of namespace details
//------------------------------------------------------------------------------

/**
 * the default printAs
 * you want to overload for your own type "MyType" like this:
 *
 *  template<size_t BASE, char BASE_CHAR = 'A', typename OStream>
 *  inline void printAs(OStream& oStream, const MyType& value, unsigned int digits)
 *  { }
 *
 * NOTE: we don't need the "enable_ifNumber", because the compiler would fail
 *       with some cryptic message basically meaning there is no "make_unsigned"
 *       for "not number" types.
 *       With this enable_ifNumber we get: "no matching function for call to 'printAs'"
 *       Which is exactly what we want to tell our users
 */
template<size_t BASE, char BASE_CHAR = 'A', typename OStream, typename NumType,
        typename = details::enableIfNumber<NumType> >
inline void printAs(OStream& oStream, const NumType& value, unsigned int digits)
{
    using Type = typename std::decay<NumType>::type;
    using UType = typename std::make_unsigned<Type>::type;

    print_unsigned<BASE, BASE_CHAR>(oStream, static_cast<UType>(value), digits);
}

// the helper functor for the printAs function
template<unsigned char BASE, char BASE_CHAR = 'A', typename T>
inline auto callPrintAs(const T& value, unsigned int digits = 0)
{
    return Printer{[&value, digits](auto& stream){
        printAs<BASE, BASE_CHAR>(stream, value, digits);
    }};
}

template<unsigned char BASE, typename NumType >
inline auto call_printAs(const NumType& Value, unsigned int Digits = 0)
{ return callPrintAs<BASE>(Value, Digits); }

template<typename T>
inline auto hex(const T& value, unsigned int Digits = 0)
{ return callPrintAs<16>(value, Digits); }

template<typename T >
inline auto hex_lowercase(const T& value, unsigned int Digits = 0)
{ return callPrintAs<16, 'a'>(value, Digits); }

template<typename T >
inline auto hex_uppercase(const T& value, unsigned int Digits = 0)
{ return callPrintAs<16, 'A'>(value, Digits); }

template<typename T>
inline auto dec(const T& value, unsigned int Digits = 0)
{ return callPrintAs<10>(value, Digits); }

template<typename T >
inline auto bin(const T& value, unsigned int Digits = 0)
{ return callPrintAs<2>(value, Digits); }

//******************************************************************************
// allow in-stream dump of memory
template <size_t BASE, size_t line_width, size_t number_width, int line_number_width, typename IteratorType>
inline auto dump_wrapper(IteratorType ibegin, IteratorType iend)
{
    return Printer{[=](auto& stream){
        dump<BASE, line_width, number_width, line_number_width>(stream, ibegin, iend);
    }};
}

template<typename IteratorType>
inline auto hex_dump_wrapper(const IteratorType& ibegin, const IteratorType& iend)
{ return dump_wrapper<16, 16, 2, 4>(ibegin, iend); }

template<typename IteratorType>
inline auto hex_dump(const IteratorType& ibegin, const IteratorType& iend)
{ return hex_dump_wrapper(ibegin, iend); }

template<typename Container>
inline auto hex_dump(const Container& container)
{ return hex_dump_wrapper(begin(container), end(container)); }

template<typename T>
inline auto hex_dump(const T* ptr, size_t count)
{
    return hex_dump_wrapper(reinterpret_cast<const char*>(ptr),
                            reinterpret_cast<const char*>(ptr + count));
}

//------------------------------------------------------------------------------
// A little Helper to print any sequence with a separator and a callable object as printer
template<typename IteratorBegin, typename IteratorEnd,
         typename SeparatorType,
         typename PrinterType>
struct SequencePrinter
{
    using begin_t = IteratorBegin;
    using end_t = IteratorEnd;
    using separator_t = SeparatorType;
    using printer_t = PrinterType;

    begin_t     ivBegin;
    end_t       ivEnd;
    separator_t ivSeparator;
    printer_t   ivPrinter;

    template<typename StreamType>
    StreamType& printTo(StreamType& OStream) const
    {
        auto pos = ivBegin;
        if (pos != ivEnd) { ivPrinter(OStream, *pos++); }

        while(pos != ivEnd)
        {
            OStream << ivSeparator;
            ivPrinter(OStream, *pos++);
        }

        return OStream;
    }
};

template<typename IteratorBegin, typename IteratorEnd, typename SeparatorType, typename PrinterType>
inline auto createSequencePrinter(IteratorBegin&& ibegin, IteratorEnd&& iend,
                                  SeparatorType&& separator,
                                  PrinterType&& printer)
{
    return SequencePrinter<IteratorBegin, IteratorEnd, SeparatorType, PrinterType>
                   {std::forward<IteratorBegin>(ibegin),
                    std::forward<IteratorEnd>(iend),
                    std::forward<SeparatorType>(separator),
                    std::forward<PrinterType>(printer)};
}

//******************************************************************************
// print a sequence starting at *begin and ending one before *end
// the iterator type requires only the operators !=, *, post++
// You want to use it like:
// std::cout << StreamFunc::seq(MyVec.begin(), MyVec.end(), "...") << std::endl;
// std::cout << StreamFunc::seq(MyVec) << std::endl;
//------------------------------------------------------------------------------
// print a sequence using a callable object to print the objects in the container
// starting at *begin and ending one before *end
// the iterator type requires only the operators !=, *, post++
// You want to use it like:
// std::cout << seq_as(MyVec, printer::number<8>(3)) << std::endl;
template<typename IteratorBegin, typename IteratorEnd, typename PrinterType, typename SeparatorType>
inline auto seq_as(IteratorBegin&& ibegin, IteratorEnd&& iend, PrinterType&& printer,
                   SeparatorType&& separator)
{
    return Printer{createSequencePrinter(std::forward<IteratorBegin>(ibegin),
                                 std::forward<IteratorEnd>(iend),
                                 std::forward<SeparatorType>(separator),
                                 std::forward<PrinterType>(printer))};
}

template<typename ContainerType, typename PrinterType, typename SeparatorType=char>
inline auto seq_as(const ContainerType& container,
                   PrinterType&& printer,
                   SeparatorType&& separator = ',')
{
    return seq_as( begin(container), end(container),
                   std::forward<PrinterType>(printer),
                   std::forward<SeparatorType>(separator));
}

template<typename STREAM, typename IteratorBegin, typename IteratorEnd, typename SeparatorType, typename PrinterType>
inline STREAM& print_sequence_as(STREAM& stream, IteratorBegin&& ibegin, IteratorEnd&& iend,
                                 SeparatorType&& separator, PrinterType&& printer)
{
    auto out = seq_as(std::forward<IteratorBegin>(ibegin),
                      std::forward<IteratorEnd>(iend),
                      std::forward<PrinterType>(printer),
                      std::forward<SeparatorType>(separator));
    stream << out;
    return stream;
}

template<typename STREAM, typename ContainerType, typename SeparatorType, typename PrinterType>
inline STREAM& print_sequence_as(STREAM& stream, const ContainerType& container,
                                 SeparatorType&& separator, PrinterType&& printer)
{
    stream << seq_as(container, std::forward<PrinterType>(printer), std::forward<SeparatorType>(separator));
    return stream;
}

template<typename IteratorBegin, typename IteratorEnd, typename SeparatorType>
inline auto seq(IteratorBegin&& ibegin, IteratorEnd&& iend, SeparatorType&& separator)
{
    return seq_as(std::forward<IteratorBegin>(ibegin),
                  std::forward<IteratorEnd>(iend),
                  [](auto& stream, auto& e) { stream << e; },
                  std::forward<SeparatorType>(separator));
}

template<typename ContainerType, typename SeparatorType=char>
inline auto seq(const ContainerType& container, SeparatorType&& separator = ',')
{
    return seq(begin(container), end(container), std::forward<SeparatorType>(separator));
}

template<typename STREAM, typename IteratorBegin, typename IteratorEnd, typename SeparatorType>
inline STREAM& print_sequence(STREAM& stream, IteratorBegin&& ibegin, IteratorEnd&& iend, SeparatorType&& separator)
{
    auto out = seq(std::forward<IteratorBegin>(ibegin),
                   std::forward<IteratorEnd>(iend),
                   std::forward<SeparatorType>(separator));
    stream << out;
    return stream;
}

template<typename STREAM, typename ContainerType, typename SeparatorType=char>
inline STREAM& print_sequence(STREAM& stream, const ContainerType& container, SeparatorType&& separator = ',')
{
    stream << seq(container, std::forward<SeparatorType>(separator));
    return stream;
}

//------------------------------------------------------------------------------
template<typename IteratorTypeBegin, typename IteratorTypeEnd>
inline auto to_upper(IteratorTypeBegin&& begin, IteratorTypeEnd&& end)
{
    return seq_as(std::forward<IteratorTypeBegin>(begin),
                  std::forward<IteratorTypeEnd>(end),
                  printer::to_upper_wrapper{}, empty);
}

template<typename ContainerType>
inline auto to_upper(const ContainerType& container)
{ return to_upper( begin(container), end(container) ); }

template<typename IteratorTypeBegin, typename IteratorTypeEnd>
inline auto to_lower(IteratorTypeBegin&& begin, IteratorTypeEnd&& end)
{
    return seq_as(std::forward<IteratorTypeBegin>(begin),
                  std::forward<IteratorTypeEnd>(end),
                  printer::to_lower_wrapper{}, empty);
}

template<typename ContainerType>
inline auto to_lower(const ContainerType& container)
{ return to_lower( begin(container), end(container) ); }

//------------------------------------------------------------------------------
namespace details {
//------------------------------------------------------------------------------

template<typename A, typename B>
constexpr auto isSameDecayed = std::is_same_v< std::decay_t<A>, std::decay_t<B> >;

template<typename A, typename B>
using enableIfSameDecayed = std::enable_if_t< details::isSameDecayed<A, B> >;

//------------------------------------------------------------------------------
}  // namespace details
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template<unsigned char BASE, typename IteratorTypeBegin, typename IteratorTypeEnd, typename SeparatorType=char >
inline auto seq_unsigned(IteratorTypeBegin&& begin, IteratorTypeEnd&& end, unsigned int Digits = 0, SeparatorType&& separator = ',')
{
    return seq_as(std::forward<IteratorTypeBegin>(begin),
                  std::forward<IteratorTypeEnd>(end),
                  StreamFunc::printer::number<BASE>(Digits),
                  std::forward<SeparatorType>(separator));
}

template<unsigned char BASE, typename ContainerType, typename SeparatorType=char>
inline auto seq_unsigned(const ContainerType& container, unsigned int Digits = 0, SeparatorType&& separator = ',') ->
     decltype( seq_as( begin(container), end(container), Digits, std::forward<SeparatorType>(separator)) )
{
    return seq_as( begin(container), end(container), Digits, std::forward<SeparatorType>(separator));
}

template<typename IteratorTypeBegin, typename IteratorTypeEnd, typename SeparatorType=char,
        typename = details::enableIfSameDecayed<IteratorTypeBegin, IteratorTypeEnd> >
inline auto seq_hex(IteratorTypeBegin&& begin, IteratorTypeEnd&& end, unsigned int Digits = 0, SeparatorType&& separator = ',')
{
    return seq_as(std::forward<IteratorTypeBegin>(begin),
                  std::forward<IteratorTypeEnd>(end),
                  StreamFunc::printer::hex(Digits),
                  std::forward<SeparatorType>(separator));
}

template<typename ContainerType, typename SeparatorType=char>
inline auto seq_hex(const ContainerType& container, unsigned int Digits = 0, SeparatorType&& separator = ',')
{ return seq_hex( begin(container), end(container), Digits, std::forward<SeparatorType>(separator)); }

template<typename IteratorTypeBegin, typename IteratorTypeEnd, typename SeparatorType=char,
        typename = details::enableIfSameDecayed<IteratorTypeBegin, IteratorTypeEnd> >
inline auto seq_bin(IteratorTypeBegin&& begin, IteratorTypeEnd&& end, unsigned int Digits = 0, SeparatorType&& separator = ',')
{
    return seq_as(std::forward<IteratorTypeBegin>(begin),
                  std::forward<IteratorTypeEnd>(end),
                  StreamFunc::printer::bin(Digits),
                  std::forward<SeparatorType>(separator));
}

template<typename ContainerType, typename SeparatorType=char>
inline auto seq_bin(const ContainerType& container, unsigned int Digits = 0, SeparatorType&& separator = ',')
{ return seq_bin( begin(container), end(container), Digits, std::forward<SeparatorType>(separator)); }

//------------------------------------------------------------------------------
namespace details {
//------------------------------------------------------------------------------

template<typename T> struct TimeUnit {};

template<>
struct TimeUnit<std::chrono::nanoseconds>
{
    using next = void;
    static auto suffix() { return "ns"; }
};

template<>
struct TimeUnit<std::chrono::microseconds>
{
    using next = void;
    static auto suffix() { return "us"; }
};

template<>
struct TimeUnit<std::chrono::milliseconds>
{
    using next = std::chrono::microseconds;
    static auto suffix() { return "ms"; }
};

template<>
struct TimeUnit<std::chrono::seconds>
{
    using next = std::chrono::milliseconds;
    static auto suffix() { return 's'; }
};

template<>
struct TimeUnit<std::chrono::minutes>
{
    using next = std::chrono::seconds ;
    static auto suffix() { return "min"; }
};

template<>
struct TimeUnit<std::chrono::hours>
{
    using next = std::chrono::minutes;
    static auto suffix() { return 'h'; }
};

template<typename UNIT, typename STREAM, class Rep, class Period>
static void printTime(STREAM& s, const std::chrono::duration<Rep, Period>& duration)
{
    using type = TimeUnit<UNIT>;

    auto unit = std::chrono::duration_cast<UNIT>(duration);
    if (unit > UNIT::zero() )
    { s << unit.count() << type::suffix(); }

    using next = typename type::next;
    if constexpr (not std::is_void_v<next>)
    { return printTime<next>(s, duration - unit); }
}

template<size_t N>
static constexpr size_t getNumberOfDecimalDigits()
{
    if (N == 0) { return 0;}
    return 1 + getNumberOfDecimalDigits<N/10>();
}

//------------------------------------------------------------------------------
} // end of namespace details
//------------------------------------------------------------------------------
template< typename UNIT, typename REP, typename PERIOD >
inline auto time_as(std::chrono::duration<REP, PERIOD> duration)
{
    return Printer{[unit = std::chrono::duration_cast<UNIT>(duration)](auto& stream)
                   {
                       stream << unit.count() << details::TimeUnit<UNIT>::suffix();
                   }};
}

template< typename UNIT = std::chrono::hours, typename REP, typename PERIOD >
inline auto time(std::chrono::duration<REP, PERIOD> duration)
{
    return Printer{[=](auto& stream)
                   {
                       if (duration == UNIT::zero())
                       { stream << '0' << details::TimeUnit<UNIT>::suffix(); }
                       else
                       { details::printTime<UNIT>(stream, duration); }
                   }};
}

//------------------------------------------------------------------------------
template< typename REP, typename PERIOD >
inline auto time_log(std::chrono::duration<REP, PERIOD> duration)
{
    return Printer{[=](auto& stream){
        using namespace std::chrono;
        using small = microseconds;
        constexpr auto ratio = small::period::den;

        const auto sec = duration_cast<seconds>(duration);
        const auto rest = duration_cast<small>(duration- sec);
        stream << sec.count() << '.' << dec(rest.count(), details::getNumberOfDecimalDigits<ratio>()-1);
    }};
}

//------------------------------------------------------------------------------
template< typename CLOCK, typename DURATION >
inline auto time(std::chrono::time_point<CLOCK, DURATION> timepoint)
{
    using namespace std::chrono;
    using small = microseconds;
    return Printer{[stamp = duration_cast<small>(timepoint.time_since_epoch()).count()](auto& stream){
        constexpr auto ratio = small::period::den;
        time_t secs = stamp / ratio;
        auto micros = static_cast<uint64_t>(stamp % ratio);

        stream << std::put_time(std::gmtime(&secs), "%m-%d-%y %H:%M:%S");
        stream << '.' << dec(micros,details::getNumberOfDecimalDigits<ratio>()-1);
    }};
}

//******************************************************************************
}   // end of namespace StreamFunc
//******************************************************************************
