/**
 * @file RHIValidationUnwrapTests.cpp
 * @brief Tests raw, shared, nested, null, and backend-tag unwrapping.
 */
#include "Validation/RHIValidationTestUtils.hpp"
#include <Resources/NullResources.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationUnwrap, ReachesConcreteBackendThroughNestedDecorators )
    {
        Tests::FRecordingDebugSink Sink;
        auto Null                      = MakeShared<Tests::FNullDevice>();
        TSharedRef<IRHIDevice> Backend = Null;
        auto Inner                     = *WrapRHIDevice( Backend );
        auto Outer                     = *WrapRHIDevice( Inner );
        LUMEN_CHECK_EQ( UnwrapRHI( Outer.Get() ), Null.Get() );
        LUMEN_CHECK_EQ( TryUnwrapRHIBackend<Tests::FNullDevice>( Outer.Get() ), Null.Get() );
        LUMEN_CHECK_EQ( TryUnwrapRHIBackend<Tests::FNullBuffer>( Outer.Get() ), nullptr );
        const TSharedRef<IRHIObject> Base = Outer;
        const auto Shared                 = UnwrapRHISharedObject( Base );
        LUMEN_CHECK_EQ( Shared.Get(), static_cast<IRHIObject *>( Null.Get() ) );
        LUMEN_CHECK_TRUE( Shared.GetSharedReferenceCount() >= 3ULL );
    }

    LUMEN_TEST( RHIValidationUnwrap, HandlesNullSharedValuesAndDisabledValidation )
    {
        IRHIObject *NullObject = nullptr;
        LUMEN_CHECK_EQ( UnwrapRHI( NullObject ), nullptr );
        LUMEN_CHECK_EQ( UnwrapRHIObject( NullObject ), nullptr );
        TSharedPtr<IRHIBuffer> NullBuffer;
        LUMEN_CHECK_FALSE( UnwrapRHIShared( NullBuffer ).IsValid() );
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Backend = MakeShared<Tests::FNullDevice>();
        FRHIValidationDesc Desc;
        Desc.Level  = ERHIValidationLevel::Disabled;
        auto Device = *WrapRHIDevice( Backend, Desc );
        LUMEN_CHECK_EQ( Device.Get(), Backend.Get() );
        auto Buffer    = Tests::Validation::MakeBuffer( Device );
        auto Unwrapped = UnwrapRHIShared( Buffer );
        LUMEN_CHECK_EQ( Unwrapped.Get(), Buffer.Get() );
    }
} // namespace
} // namespace Lumen
