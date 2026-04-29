/**
 * @file Version.hpp
 * @brief Engine Version information
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace Version
{

    constexpr Int32 Major = 0;
    constexpr Int32 Minor = 1;
    constexpr Int32 Patch = 0;

    constexpr const char *String = "0.1.0";

    constexpr UInt32 Packed = ( Major << 16 ) | ( Minor << 8 ) | Patch;

} // namespace Version

} // namespace LumenEngine
