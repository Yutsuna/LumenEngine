/**
 * @file AlgoSortTestHelper.hpp
 * @brief Shared data generators and checkers for the Algo sorting tests.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <String/String.hpp>
#include <Templates/Array.hpp>
#include <Templates/Function.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace Tests
{

    namespace SortHelpers
    {

        /** @brief Deterministic xorshift64 generator: tests must be reproducible. */
        struct FSortRandom
        {
            explicit FSortRandom ( UInt64 InSeed ) noexcept;

            [[nodiscard]] UInt64 Next () noexcept;

        private:

            UInt64 State;
        };

        /** @brief Input distributions that stress pivot selection and partitioning. */
        enum class ESortShape : UInt8
        {
            Random,
            Sorted,
            Reversed,
            AllEqual,
            FewUnique,
            OrganPipe,
            SawTooth
        };

        /** @brief Record with a sort key, used to check payload integrity and stability. */
        struct FSortRecord
        {
            Int64 Key;
            Int64 Payload;
            Int64 Extra;
        };

        /** @brief Every shape. */
        inline constexpr TArray<ESortShape, 7> AllShapes{ { ESortShape::Random, ESortShape::Sorted, ESortShape::Reversed,
                                                            ESortShape::AllEqual, ESortShape::FewUnique, ESortShape::OrganPipe,
                                                            ESortShape::SawTooth } };

        /** @brief Sizes around the insertion threshold (16), the ninther threshold (128) and larger. */
        inline constexpr TArray<USize, 19> SortTestSizes{ { 0, 1, 2, 3, 15, 16, 17, 18, 31, 32, 33, 127, 128, 129, 130, 255, 1000,
                                                            4096, 10007 } };

        using FIntSorter    = TFunction<void( TVector<Int32> & )>;
        using FStringSorter = TFunction<void( TVector<FString> & )>;
        using FRecordSorter = TFunction<void( TVector<FSortRecord> & )>;

        /** @brief Fills OutValues with InCount integers following InShape. */
        void FillShape ( TVector<Int32> &OutValues, USize InCount, ESortShape InShape, UInt64 InSeed );

        /** @brief Fills OutRecords with random keys in [0, InKeyRange); Extra holds the original index. */
        void FillRecords ( TVector<FSortRecord> &OutRecords, USize InCount, UInt64 InKeyRange, UInt64 InSeed );

        /** @brief Checks the sorter on one (size, shape) case against std::ranges::sort. */
        void CheckSorterOnShape ( const FIntSorter &InSorter, USize InCount, ESortShape InShape, Bool bDescending = false );

        /** @brief Checks the sorter on every shape for every size of SortTestSizes up to InMaxCount. */
        void CheckSorterOnAllShapes ( const FIntSorter &InSorter, Bool bDescending = false, USize InMaxCount = 10007 );

        /** @brief Checks the sorter on every shape for every size in [0, 70] (even/odd counts, every threshold edge). */
        void CheckSorterOnSmallSizes ( const FIntSorter &InSorter, Bool bDescending = false );

        /** @brief Checks the sorter on random strings (non-trivial value type). */
        void CheckStringSorter ( const FStringSorter &InSorter );

        /** @brief Checks that the sorter orders records by ascending Key and loses none. */
        void CheckRecordSorter ( const FRecordSorter &InSorter );

    } // namespace SortHelpers

} // namespace Tests

} // namespace Lumen
