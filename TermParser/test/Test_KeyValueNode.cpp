#include "TermParser/keyvaluenode.hpp"

#include <string_view>

#include <gtest/gtest.h>

//******************************************************************************
namespace {
//------------------------------------------------------------------------------
using Node = mmc::KeyValueNode<char, int>;

//------------------------------------------------------------------------------
} // end of anonymous namespace
//------------------------------------------------------------------------------

TEST(Test_KeyValueNode, simple)
{
    Node root;

    root.emplace("a", 1);
    root.emplace("b", 2);
    root.emplace("aa", 11);
    root.emplace("ab", 12);
    root.emplace("ba", 21);
    root.emplace("bb", 22);
    root.emplace("abc", 123);
    root.emplace("abab", 1212);

    EXPECT_EQ(1, root.getOr("a",-1));
    EXPECT_EQ(2, root.getOr("b",-1));
    EXPECT_EQ(11, root.getOr("aa",-1));
    EXPECT_EQ(12, root.getOr("ab",-1));
    EXPECT_EQ(21, root.getOr("ba",-1));
    EXPECT_EQ(22, root.getOr("bb",-1));
    EXPECT_EQ(123, root.getOr("abc",-1));

    EXPECT_EQ(-1, root.getOr("",-1));
    EXPECT_EQ(-1, root.getOr("c",-1));
    EXPECT_EQ(-1, root.getOr("ca",-1));
    EXPECT_EQ(12, root.getOr("abd",-1));
    EXPECT_EQ(123, root.getOr("abcd",-1));
    EXPECT_EQ(1212, root.getOr("abab",-1));

    EXPECT_EQ(12, root.getOr(std::string_view("aba"),-1));

} // simple

#define PARSE_NEXT(m_node, exp_value, exp_char) {       \
    auto [next_pos, value_ptr] = root.find(pos, end);   \
    pos = std::move(next_pos);                          \
    ASSERT_TRUE(value_ptr);                             \
    EXPECT_EQ( (exp_value), *value_ptr);                \
    if ( (exp_char) == '\0') { EXPECT_EQ(pos, end); }   \
    else { EXPECT_EQ( (exp_char), *pos); }              \
}

TEST( Test_KeyValueNode, parsing )
{
    Node root;
    root.emplace("first",  1);
    root.emplace("second", 2);
    root.emplace("+",      3);
    root.emplace("*",      4);
    root.emplace("-",      5);
    root.emplace("fir",    6);
    root.emplace("sec",    7);

    const auto& term = "first+second*fir-sec";
    auto pos = mmc::begin(term);
    auto end = mmc::end(term);

    PARSE_NEXT(root, 1, '+');
    PARSE_NEXT(root, 3, 's');
    PARSE_NEXT(root, 2, '*');
    PARSE_NEXT(root, 4, 'f');
    PARSE_NEXT(root, 6, '-');
    PARSE_NEXT(root, 5, 's');
    PARSE_NEXT(root, 7, '\0');
}   // TEST parsing
