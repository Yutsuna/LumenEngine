/**
 * @file CoreTypes.hpp
 * @brief Core type definitions for LumenEngine
 */

#pragma once

namespace LumenEngine
{

using Int8 = signed char;
using Int16 = short;
using Int32 = int;
using Int64 = long long;

using Uint8 = unsigned char;
using Uint16 = unsigned short;
using Uint32 = unsigned int;
using Uint64 = unsigned long long;

using Float32 = float;
using Float64 = double;

using AnsiChar = char;
using WideChar = wchar_t;
using Bool = bool;
using Byte = unsigned char;

/**
 * Usize: Unsigned integer type used for sizes and indexing
 *
 * @brief Calculated using compiler intrinsics
 */
using Usize = decltype( sizeof( 0 ) );

/**
 * Isize: Signed integer type used for pointer arithmetic and offsets
 *
 * @brief Calculated using compiler intrinsics
 */
using Isize = decltype( ( AnsiChar * )0 - ( AnsiChar * )0 );

/**
 * Uptr / Iptr: Integers guaranteed to be the same size as a pointer
 */
#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __aarch64__ ) || defined( __LP64__ )
using Uptr = unsigned long long;
using Iptr = long long;
#else
using Uptr = unsigned int;
using Iptr = int;
#endif

} // namespace LumenEngine
