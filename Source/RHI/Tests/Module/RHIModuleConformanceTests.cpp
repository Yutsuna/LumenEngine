/**
 * @file RHIModuleConformanceTests.cpp
 * @brief Runs portable module discovery on the linked backend.
 */

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {

        LUMEN_TEST( RHIModuleConformance, EnumeratesAtLeastOneUsableAdapter )
        {
            const TSharedRef<IRHIModule> Module = Tests::GetRHIConformanceDeviceFactory().GetModule();
            LUMEN_CHECK_EQ( Module->GetApiVersion().Major, 1U );
            LUMEN_CHECK_EQ( Module->GetApiVersion().Minor, 1U );
            LUMEN_CHECK_EQ( Module->GetApiVersion().Patch, 0U );

            const TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = Module->EnumerateAdapters();
            LUMEN_CHECK_TRUE( Adapters.HasValue() );
            LUMEN_CHECK_FALSE( Adapters->empty() );
            LUMEN_CHECK_TRUE( ( *Adapters )[0].Validate().HasValue() );
        }

        LUMEN_TEST( RHIModuleConformance, SelectsAdaptersAndRejectsUnknownIdentity )
        {
            const TSharedRef<IRHIModule> Module                           = Tests::GetRHIConformanceDeviceFactory().GetModule();
            const TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = Module->EnumerateAdapters();
            LUMEN_CHECK_TRUE( Adapters.HasValue() );
            LUMEN_CHECK_FALSE( Adapters->empty() );

            FRHIDeviceDesc Desc;
            Desc.AdapterId.Emplace( ( *Adapters )[0].AdapterId );
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Module->CreateDevice( Desc );
            LUMEN_CHECK_TRUE( Device.HasValue() );
            LUMEN_CHECK_EQ( ( *Device )->GetStatus(), ERHIDeviceStatus::Ready );
            LUMEN_CHECK_TRUE( ( *Device )->GetCapabilities().Supports( Desc.RequiredCapabilities.Features ) );

            Desc.AdapterId.Emplace( ~( *Adapters )[0].AdapterId );
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> Missing = Module->CreateDevice( Desc );
            LUMEN_CHECK_HAS_ERROR( Missing );
            LUMEN_CHECK_EQ( Missing.Error().Code, ERHIErrorCode::NotFound );
            LUMEN_CHECK_EQ( Missing.Error().Condition, ERHIErrorCondition::Device );
        }

        LUMEN_TEST( RHIModuleConformance, RejectsUnavailableCapabilitiesAndExcessQueues )
        {
            const TSharedRef<IRHIModule> Module                           = Tests::GetRHIConformanceDeviceFactory().GetModule();
            const TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = Module->EnumerateAdapters();
            LUMEN_CHECK_TRUE( Adapters.HasValue() );
            LUMEN_CHECK_FALSE( Adapters->empty() );
            const FRHIAdapterInfo &Adapter = ( *Adapters )[0];

            FRHIDeviceDesc QueueDesc;
            QueueDesc.Queues = { { .Type = ERHIQueueType::Graphics, .Count = 0U } };
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> InvalidQueue = Module->CreateDevice( QueueDesc );
            LUMEN_CHECK_HAS_ERROR( InvalidQueue );
            LUMEN_CHECK_EQ( InvalidQueue.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( InvalidQueue.Error().Condition, ERHIErrorCondition::Queue );

            QueueDesc.Queues = { { .Type = ERHIQueueType::Graphics, .Count = 0xFFFFFFFFU } };
            const TExpected<TSharedRef<IRHIDevice>, FRHIError> ExcessQueue = Module->CreateDevice( QueueDesc );
            LUMEN_CHECK_HAS_ERROR( ExcessQueue );
            LUMEN_CHECK_EQ( ExcessQueue.Error().Code, ERHIErrorCode::Unsupported );
            LUMEN_CHECK_EQ( ExcessQueue.Error().Condition, ERHIErrorCondition::Queue );

            const TArray<ERHICapabilityFlags, 3U> OptionalFeatures = {
                ERHICapabilityFlags::RayTracing, ERHICapabilityFlags::MeshShaders, ERHICapabilityFlags::BindlessResources };
            for ( const ERHICapabilityFlags Feature : OptionalFeatures )
            {
                if ( Adapter.Capabilities.Supports( Feature ) )
                {
                    continue;
                }
                FRHIDeviceDesc FeatureDesc;
                FeatureDesc.RequiredCapabilities.Features                      = Feature;
                const TExpected<TSharedRef<IRHIDevice>, FRHIError> Unsupported = Module->CreateDevice( FeatureDesc );
                LUMEN_CHECK_HAS_ERROR( Unsupported );
                LUMEN_CHECK_EQ( Unsupported.Error().Code, ERHIErrorCode::Unsupported );
                LUMEN_CHECK_EQ( Unsupported.Error().Condition, ERHIErrorCondition::Capability );
                return;
            }
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
