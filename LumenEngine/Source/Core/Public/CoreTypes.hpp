/**
 * @file CoreTypes.hpp
 * @brief Core type definitions for LumenEngine
 */

#pragma once

#include <atomic>
#include <expected>

namespace LumenEngine
{

using Int8  = signed char;
using Int16 = short;
using Int32 = int;
using Int64 = long long;

using UInt8  = unsigned char;
using UInt16 = unsigned short;
using UInt32 = unsigned int;
using UInt64 = unsigned long long;

using Float32 = float;
using Float64 = double;

using AnsiChar = char;
using WideChar = wchar_t;
using Bool     = bool;
using Byte     = unsigned char;

// TODO: Implement custom atomic type
template <typename Type> using TAtomic                   = std::atomic<Type>;
using FAtomicFlag                                        = std::atomic_flag;
template <typename Type, typename Error> using TExpected = std::expected<Type, Error>;

using NullptrType = decltype( nullptr );

/**
 * USize: Unsigned integer type used for sizes and indexing
 *
 * @brief Calculated using compiler intrinsics
 */
using USize = decltype( sizeof( 0 ) );

/**
 * Isize: Signed integer type used for pointer arithmetic and offsets
 *
 * @brief Calculated using compiler intrinsics
 */
using ISize = decltype( static_cast<AnsiChar *>( nullptr ) - static_cast<AnsiChar *>( nullptr ) );

/**
 * Uptr / Iptr: Integers guaranteed to be the same size as a pointer
 */
#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __aarch64__ ) || defined( __LP64__ )
using UPtr = unsigned long long;
using IPtr = long long;
#else
using Uptr = unsigned int;
using Iptr = int;
#endif

} // namespace LumenEngine
