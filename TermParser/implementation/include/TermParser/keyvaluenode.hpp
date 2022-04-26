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
#pragma once

#include <optional>
#include <memory>
#include <unordered_map>
#include <cstring>

//******************************************************************************
namespace mmc {
//******************************************************************************

//------------------------------------------------------------------------------
namespace details {
//------------------------------------------------------------------------------

template<typename T>
struct Size
{
    static auto begin(const T& value) { return std::begin(value); }
    static auto end(const T& value) { return std::end(value); }
};

template<size_t N>
struct Size<char [N]>
{
    static const char* begin(const char (&arr)[N]) { return arr; }
    static const char* end(const char (&arr)[N]) { return arr+N-1; }
};

template<>
struct Size<const char*>
{
    static const char*  begin(const char* str) { return str; }
    static const char*  end(const char* str) { return str + strlen(str); }
};

//------------------------------------------------------------------------------
} // end of namespace details
//------------------------------------------------------------------------------

template<typename T>
inline auto begin(const T& str) { return details::Size<T>::begin(str); }

template<typename T>
inline auto end(const T& str) { return details::Size<T>::end(str); }

//------------------------------------------------------------------------------
template<typename KEY, typename VALUE>
class KeyValueNode
{
public:
    using key_t = KEY;
    using value_t = VALUE;
    using opt_t = std::optional<value_t>;
    using self_ptr = std::unique_ptr<KeyValueNode>;
    using map_t = std::unordered_map<key_t, self_ptr>;

    template<typename ITERATOR>
    using FindResult = std::pair< std::remove_reference_t<ITERATOR>, const value_t*>;

private:
    opt_t       ivValue;
    map_t       ivChildren;

    KeyValueNode& getChild(const key_t& key)
    {
        auto pos = ivChildren.find(key);
        if (pos == ivChildren.end())
        {
            auto [new_pos, _] = ivChildren.emplace(key, std::make_unique<KeyValueNode>());
            pos = std::move(new_pos);
        }
        return *(pos->second);
    }

    template<typename BEGIN, typename END, typename... ARGS>
    void _emplace(BEGIN&& begin, END&& end, ARGS&&... args)
    {
        if (begin != end)
        {
            auto& child = getChild(*begin);
            child._emplace(++begin, std::forward<END>(end), std::forward<ARGS>(args)...);
            return;
        }
        ivValue = value_t{ std::forward<ARGS>(args)... };
    }

public:
    explicit KeyValueNode() = default;

    template<typename CONTAINER, typename... ARGS>
    void emplace(CONTAINER&& container, ARGS&&... args)
    {
        _emplace(mmc::begin(container), mmc::end(container), std::forward<ARGS>(args)...);
    }

    template<typename BEGIN, typename END>
    FindResult<BEGIN> find(BEGIN&& begin, END&& end) const
    {
        FindResult<BEGIN> result{begin, ivValue ? &(ivValue.value()) : nullptr};
        if (begin == end) { return result; }

        auto childPos = ivChildren.find(*begin);
        if (childPos == ivChildren.end()) { return result; }

        auto leaf = childPos->second->find(++begin, std::forward<END>(end));
        if (leaf.second) { result = std::move(leaf); }
        return result;
    }

    template<typename CONTAINER>
    auto find(const CONTAINER& container) const
    {
        return find( mmc::begin(container), mmc::end(container));
    }

    template<typename CONTAINER>
    auto& getOr(const CONTAINER& container, const value_t& default_value) const
    {
        auto [ _, val_ptr ] = find(container);
        if (val_ptr) { return *val_ptr; }
        return default_value;
    }
};

//******************************************************************************
} // end of namespace mmc
//******************************************************************************
// EOF
//******************************************************************************