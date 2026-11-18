/**
 * @file RHIBindingsStressTests.cpp
 * @brief Stresses serialized binding writes and descriptor range recycling.
 */

#include <Device/NullDevice.hpp>
#include <HAL/LockGuard.hpp>
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Bindings/IRHIDescriptorAllocator.hpp>
#include <RHI/Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>
#include <Utilities/RHITestRandom.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIBindingsStress, ConcurrentWritesPreserveAValidSetAndDirtyState )
        {
            Tests::FNullDevice Device;
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            TSharedRef<IRHIBindingLayout> Layout = *Device.CreateBindingLayout( LayoutDesc );
            TSharedRef<IRHIBindingSet> Set       = *Device.CreateBindingSet( Layout );
            TSharedRef<IRHIBuffer> Buffer =
                *Device.CreateBuffer( { .Size = 256ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            Set->ClearDirty();
            TAtomic<UInt32> Failures{ 0U };
            TVector<FThread> Workers;
            for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
            {
                Workers.emplace_back( [&Set, &Buffer, &Failures, Worker]
                {
                    for ( UInt32 Index = 0U; Index < 256U; ++Index )
                    {
                        FRHIBindingWrite Write;
                        Write.Slot = 0U;
                        Write.Resource =
                            FRHIBufferBinding{ .Buffer = Buffer, .Offset = static_cast<UInt64>( Worker ) * 64ULL, .Size = 64ULL };
                        if ( not Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ) )
                        {
                            Failures.fetch_add( 1U );
                        }
                    }
                } );
            }
            for ( auto &Worker : Workers )
            {
                Worker.Join();
            }
            LUMEN_CHECK_EQ( Failures.load(), 0U );
            LUMEN_CHECK_TRUE( Set->IsDirty() );
            Set->ClearDirty();
            LUMEN_CHECK_FALSE( Set->IsDirty() );
        }

        LUMEN_TEST( RHIBindingsStress, ConcurrentDescriptorRangesNeverOverlapAndRecycle )
        {
            Tests::FNullDevice Device;
            TSharedRef<IRHIDescriptorAllocator> Allocator =
                *Device.CreateDescriptorAllocator( { .InitialCapacity = 256U, .MaximumCapacity = 256U } );
            TVector<FRHIDescriptorAllocation> Registry;
            HAL::FPlatformRecursiveSharedMutex RegistryMutex;
            TAtomic<UInt32> Failures{ 0U };
            TVector<FThread> Workers;
            for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
            {
                Workers.emplace_back( [&Allocator, &Registry, &RegistryMutex, &Failures, Worker]
                {
                    Tests::FRHITestRandom Random( 0x4445534352495054ULL + Worker );
                    TVector<FRHIDescriptorAllocation> Active;
                    for ( UInt32 Step = 0U; Step < 256U; ++Step )
                    {
                        if ( not Active.empty() and Random.Next( 3ULL ) == 0ULL )
                        {
                            const USize Index                         = static_cast<USize>( Random.Next( Active.size() ) );
                            const FRHIDescriptorAllocation Allocation = Active[Index];
                            {
                                HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( RegistryMutex );
                                for ( USize RegistryIndex = 0U; RegistryIndex < Registry.size(); ++RegistryIndex )
                                {
                                    if ( Registry[RegistryIndex].Offset == Allocation.Offset and
                                         Registry[RegistryIndex].Count == Allocation.Count )
                                    {
                                        Registry.erase( Registry.begin() + static_cast<ISize>( RegistryIndex ) );
                                        break;
                                    }
                                }
                            }
                            if ( not Allocator->Free( Allocation ) )
                            {
                                Failures.fetch_add( 1U );
                            }
                            Active.erase( Active.begin() + static_cast<ISize>( Index ) );
                        }
                        else
                        {
                            TExpected<FRHIDescriptorAllocation, FRHIError> Allocation =
                                Allocator->Allocate( static_cast<UInt32>( Random.Next( 8ULL ) + 1ULL ) );
                            if ( Allocation )
                            {
                                HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( RegistryMutex );
                                for ( const FRHIDescriptorAllocation &Other : Registry )
                                {
                                    if ( RHIRangesOverlap( Allocation->Offset, Allocation->Count, Other.Offset, Other.Count ) )
                                    {
                                        Failures.fetch_add( 1U );
                                    }
                                }
                                Registry.push_back( *Allocation );
                                Active.push_back( *Allocation );
                            }
                        }
                    }
                    for ( const FRHIDescriptorAllocation &Allocation : Active )
                    {
                        {
                            HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( RegistryMutex );
                            for ( USize RegistryIndex = 0U; RegistryIndex < Registry.size(); ++RegistryIndex )
                            {
                                if ( Registry[RegistryIndex].Offset == Allocation.Offset and
                                     Registry[RegistryIndex].Count == Allocation.Count )
                                {
                                    Registry.erase( Registry.begin() + static_cast<ISize>( RegistryIndex ) );
                                    break;
                                }
                            }
                        }
                        if ( not Allocator->Free( Allocation ) )
                        {
                            Failures.fetch_add( 1U );
                        }
                    }
                } );
            }
            for ( auto &Worker : Workers )
            {
                Worker.Join();
            }
            LUMEN_CHECK_TRUE( Registry.empty() );
            LUMEN_CHECK_EQ( Failures.load(), 0U );
            TExpected<FRHIDescriptorAllocation, FRHIError> Full = Allocator->Allocate( 256U );
            LUMEN_CHECK_TRUE( Full.HasValue() );
            LUMEN_CHECK_HAS_ERROR( Allocator->Allocate( 1U ) );
            LUMEN_CHECK_TRUE( Allocator->Free( *Full ).HasValue() );
            Allocator->Reset();
            TExpected<FRHIDescriptorAllocation, FRHIError> AfterReset = Allocator->Allocate( 256U );
            LUMEN_CHECK_TRUE( AfterReset.HasValue() );
            LUMEN_CHECK_TRUE( Allocator->Free( *AfterReset ).HasValue() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
