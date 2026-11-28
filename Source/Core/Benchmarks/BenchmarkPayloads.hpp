/**
 * @file BenchmarkPayloads.hpp
 * @brief Shared payload types for the Core template and algorithm benchmarks.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Templates/Array.hpp>

namespace Lumen
{

namespace Benchmarks
{

    namespace Internal
    {

        /** @brief Observability sink: makes every payload destruction impossible to optimise away. */
        inline volatile Int32 GPayloadSink = 0;

        /** @brief Counts copies of FCopyOnlyPayload; an atomic RMW cannot be optimised away. */
        inline TAtomic<Int32> GCopyCount{ 0 };

    } // namespace Internal

    /**
     * @class FHeavyPayload
     * @brief Payload with a 512-byte buffer and user-defined copy/move constructors.
     * @details The constructors are explicit so the compiler cannot elide the
     *          operations whose cost the audit asks to quantify.
     */
    class FHeavyPayload final
    {
    public:

        /** @brief Fills the buffer from Seed so payloads are distinct and opaque. */
        explicit FHeavyPayload ( const Int32 InSeed ) noexcept : Value( InSeed )
        {
            Int32 Offset = 0;
            for ( auto &Element : Buffer )
            {
                Element = InSeed + Offset;
                ++Offset;
            }
        }

        /** INFO: The buffer copy is deliberate — this is the cost under measurement. */
        FHeavyPayload ( const FHeavyPayload &Other ) noexcept = default;

        /** INFO: std::array move is an element-wise copy for Int32, i.e. a real 512-byte move. */
        FHeavyPayload ( FHeavyPayload &&Other ) noexcept : Value( Other.Value ), Buffer( Other.Buffer )
        {
            /** Empty */
        }

        /**
         * @brief Folds the buffer into a volatile sink so copies and moves stay observable.
         * @details Without this, GCC removes the very operations the audit asks to measure.
         */
        ~FHeavyPayload () noexcept
        {
            Int32 Checksum = 0;

            for ( const Int32 Element : Buffer )
            {
                Checksum += Element;
            }
            Internal::GPayloadSink = Checksum + Value;
        }

        /** @return The scalar used by predicates and callbacks. */
        [[nodiscard]] Int32 GetValue () const noexcept
        {
            return Value;
        }

        /** @return True when the stored scalar equals InValue. */
        [[nodiscard]] Bool Matches ( const Int32 InValue ) const noexcept
        {
            return Value == InValue;
        }

    private:

        Int32 Value;
        TArray<Int32, 128U> Buffer{};
    };

    /**
     * @class FCopyOnlyPayload
     * @brief Payload whose copy is observably more expensive than its move.
     * @details Mirrors shared ownership: copying increments an atomic counter,
     *          moving just transfers the scalar. Used for the OrElse lvalue
     *          copy (F10) where no allocation may be measured.
     */
    class FCopyOnlyPayload final
    {
    public:

        explicit FCopyOnlyPayload ( const Int32 InSeed ) noexcept : Value( InSeed )
        {
            /** Empty */
        }

        FCopyOnlyPayload ( const FCopyOnlyPayload &Other ) noexcept : Value( Other.Value )
        {
            Internal::GCopyCount.fetch_add( 1, std::memory_order_relaxed );
        }

        FCopyOnlyPayload ( FCopyOnlyPayload &&Other ) noexcept : Value( Other.Value )
        {
            /** Empty */
        }

        /** @return The stored scalar. */
        [[nodiscard]] Int32 GetValue () const noexcept
        {
            return Value;
        }

    private:

        Int32 Value;
    };

} // namespace Benchmarks

} // namespace Lumen
