/**
 * @file String.hpp
 * @brief Declaration of the FString class for string manipulation.
 */

#pragma once

// TODO: own implementation of FString
#include <string>

namespace LumenEngine
{

// TODO: own implementation of FString
using FString     = std::string;
using FStringView = std::string_view;

static inline FString StringViewToString ( FStringView InView )
{
    return { InView.data(), InView.size() };
}

} // namespace LumenEngine
