/**
 * @file Fnv1a.hpp
 * @brief FNV-1a hash implementation.
 *
 * FNV-1a is a non-cryptographic hash function for short keys.
 * Default Hash Algorithm used in LumenEngine.
 *
 */

#pragma once

#include "Concepts/Hash.hpp"

namespace LumenEngine
{

namespace Hash
{

    /**
     * @class FFnv1a64
     * @brief Stateful incremental FNV-1a 64-bit hash algorithm
     *
     * @code
     *   FFnv1a64 Algorithm {};
     *   Algorithm.Write(Pointer, Size);
     *   FHashValue Hash = Algorithm.Digest();
     * @endcode
     */
    class FFnv1a64 final
    {
    public:

        /** FNV-1a 64-bit offset basis (Fowler–Noll–Vo). */
        static constexpr FHashValue OffsetBasis = 0xCBF29CE484222325ULL;

        /** FNV-1a 64-bit prime. */
        static constexpr FHashValue Prime = 0x100000001B3ULL;

    public:

        /**
         * @brief Constructs a FFnv1a64 with the default offset basis as the initial state
         */
        constexpr FFnv1a64 () noexcept;

        /**
         * @brief Constructs a FFnv1a64 with a custom seed
         * @param InSeed The initial hash state to use instead of OffsetBasis
         */
        explicit constexpr FFnv1a64 ( const FHashValue InSeed ) noexcept;

    public:

        /**
         * @brief Ingest InSize bytes starting at InData into the running hash
         * @param InData Pointer to the data ( must not be null if InSize > 0 )
         * @param InSize Number of bytes to ingest
         */
        constexpr void Write ( const void *InData, const USize InSize ) noexcept;

        /**
         * @brief Return the current hash value without resetting stage.
         * @return 64-bit FNV-1a digest.
         */
        [[nodiscard]] constexpr FHashValue Digest () const noexcept;

        /**
         * @brief Resets the hash state to the initial offset basis
         * @param InSeed Optional new seed; defaults to the standard offset basis.
         */
        constexpr void Reset ( const FHashValue InSeed = OffsetBasis ) noexcept;

    private:

        FHashValue HashState;
    };

} // namespace Hash

} // namespace LumenEngine

#include "Inline/Fnv1a.inl"
