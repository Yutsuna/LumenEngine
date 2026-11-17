/**
 * @file RHIHashFuzzTests.cpp
 * @brief Fuzzes hash stability, trivial collisions, and typed identifiers.
 */

#include <Algo/Sort.hpp>
#include <HAL/LockGuard.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>
#include <Types/RHIHash.hpp>
#include <Types/RHIIdentifiers.hpp>
#include <Utilities/RHITestRandom.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIHashFuzz, RandomCorpusIsStableAndHasNoTrivialCollisions )
    {
        Tests::FRHITestRandom Random( 0x48415348425546ULL );
        TVector<UInt64> Hashes;
        TVector<TVector<Byte>> Corpus;
        for ( UInt32 Iteration = 0U; Iteration < 512U; ++Iteration )
        {
            TVector<Byte> Bytes( static_cast<USize>( Random.Next( 65ULL ) ) );
            for ( Byte &Value : Bytes )
            {
                Value = static_cast<Byte>( Random.Next( 256ULL ) );
            }
            const UInt64 Hash = RHIHashBytes( Bytes );
            LUMEN_CHECK_EQ( Hash, RHIHashBytes( Bytes ) );
            for ( USize Index = 0U; Index < Hashes.size(); ++Index )
            {
                if ( Hashes[Index] == Hash )
                {
                    LUMEN_CHECK_TRUE( Corpus[Index] == Bytes );
                }
            }
            Hashes.push_back( Hash );
            Corpus.push_back( Bytes );

            if ( not Bytes.empty() )
            {
                auto Mutated      = Bytes;
                const USize Index = static_cast<USize>( Random.Next( Mutated.size() ) );
                Mutated[Index]    = static_cast<Byte>( Mutated[Index] ^ 1U );
                LUMEN_CHECK_NOT_EQ( Hash, RHIHashBytes( Mutated ) );
            }
        }
    }

    LUMEN_TEST( RHIHashFuzz, TypedIdentifierHashesAreStableAcrossRandomValues )
    {
        Tests::FRHITestRandom Random( 0x4944454E54495459ULL );
        TVector<UInt64> Hashes;
        for ( UInt32 Iteration = 0U; Iteration < 1024U; ++Iteration )
        {
            const FRHIObjectId Id{ Random.Next() };
            const FRHIObjectId Copy = Id;
            const UInt64 Hash       = RHIHashValue( Id );
            LUMEN_CHECK_EQ( Id.IsValid(), Id.GetValue() != 0ULL );
            LUMEN_CHECK_TRUE( Id == Copy );
            LUMEN_CHECK_EQ( Hash, RHIHashValue( Copy ) );
            LUMEN_CHECK_EQ( static_cast<UInt64>( TRHIValueHasher<FRHIObjectId>{}( Id ) ), Hash );
            for ( const UInt64 Other : Hashes )
            {
                LUMEN_CHECK_NOT_EQ( Hash, Other );
            }
            Hashes.push_back( Hash );
        }
    }

    LUMEN_TEST( RHIHashFuzz, IdentifierGeneratorIsUniqueAndMonotonicUnderConcurrency )
    {
        FRHIIdentifierGenerator Generator;
        TVector<UInt64> Values;
        HAL::FPlatformRecursiveSharedMutex ValuesMutex;
        TAtomic<UInt32> Failures{ 0U };
        TVector<FThread> Workers;
        for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
        {
            Workers.emplace_back( [&Generator, &Values, &ValuesMutex, &Failures]
            {
                UInt64 Previous = 0ULL;
                for ( UInt32 Index = 0U; Index < 256U; ++Index )
                {
                    const FRHIObjectId Id = Generator.Next<FRHIObjectIdTag>();
                    if ( not Id.IsValid() or Id.GetValue() <= Previous )
                    {
                        Failures.fetch_add( 1U );
                    }
                    Previous = Id.GetValue();
                    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( ValuesMutex );
                    Values.push_back( Id.GetValue() );
                }
            } );
        }
        for ( auto &Worker : Workers )
        {
            Worker.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Values.size(), 1024U );
        Algo::Sort( Values );
        for ( USize Index = 0U; Index < Values.size(); ++Index )
        {
            LUMEN_CHECK_EQ( Values[Index], static_cast<UInt64>( Index ) + 1ULL );
        }
    }

} // namespace
} // namespace Lumen
