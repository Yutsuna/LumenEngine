/**
 * @file Endian.hpp
 * @brief Endian utilities for byte manipulation.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Concepts/TriviallyCopyable.hpp"
#include "Container/Vector.hpp"

#include <bit>

namespace LumenEngine
{

namespace Bytes
{

    using Endian = std::endian;

    /**
     * @brief Writes a trivially copyable value to a byte buffer with specified endianness.
     * @tparam Type The type of the value to write, must be trivially copyable.
     * @param OutBuffer The byte buffer to write to.
     * @param Value The value to write to the buffer.
     * @param ByteOrder The endianness to use when writing the value (default is native endianness).
     */
    template <typename Type>
        requires Concepts::CTriviallyCopyable<Type>
    void Write ( TVector<Byte> &OutBuffer, const Type Value, const Endian ByteOrder = Endian::native ) noexcept;

    /**
     * @brief Writes a trivially copyable value to a byte buffer in little-endian format.
     * @tparam Type The type of the value to write, must be trivially copyable.
     * @param OutBuffer The byte buffer to write to.
     * @param Value The value to write to the buffer.
     */
    template <typename Type>
        requires Concepts::CTriviallyCopyable<Type>
    void WriteLittleEndian ( TVector<Byte> &OutBuffer, const Type Value ) noexcept;

    /**
     * @brief Writes a trivially copyable value to a byte buffer in big-endian format.
     * @tparam Type The type of the value to write, must be trivially copyable.
     * @param OutBuffer The byte buffer to write to.
     * @param Value The value to write to the buffer.
     */
    template <typename Type>
        requires Concepts::CTriviallyCopyable<Type>
    void WriteBigEndian ( TVector<Byte> &OutBuffer, const Type Value ) noexcept;

    //----------

    /**
     * @brief Reads a trivially copyable value from a byte buffer with specified endianness.
     * @tparam Type The type of the value to read, must be trivially copyable
     * @param InPtr The byte buffer to read from.
     * @param ByteOrder The endianness to use when reading the value (default is native endianness).
     * @return The value read from the buffer.
     */
    template <typename Type>
        requires Concepts::CTriviallyCopyable<Type>
    Type Read ( const Byte *InPtr, const Endian ByteOrder = Endian::native ) noexcept;

    /**
     * @brief Reads a trivially copyable value from a byte buffer in little-endian format.
     * @tparam Type The type of the value to read, must be trivially copyable
     * @param InPtr The byte buffer to read from.
     * @return The value read from the buffer.
     */
    template <typename Type>
        requires Concepts::CTriviallyCopyable<Type>
    Type ReadLittleEndian ( const Byte *InPtr ) noexcept;

    /**
     * @brief Reads a trivially copyable value from a byte buffer in big-endian format.
     * @tparam Type The type of the value to read, must be trivially copyable
     * @param InPtr The byte buffer to read from.
     * @return The value read from the buffer.
     */
    template <typename Type>
        requires Concepts::CTriviallyCopyable<Type>
    Type ReadBigEndian ( const Byte *InPtr ) noexcept;

} // namespace Bytes

} // namespace LumenEngine

#include "Inline/Endian.inl"
