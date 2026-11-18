/**
 * @file RHIErrorTests.cpp
 * @brief Tests structured errors and monadic propagation.
 */

#include <RHI/Types/RHIError.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIError, PreservesCauseAndAddsContext )
    {
        const FRHIError Error( ERHIErrorCode::OutOfMemory, ERHIErrorCondition::Memory, "heap full",
                               TOptional<Int64>( static_cast<Int64>( 42 ) ) );
        const FRHIError Context = Error.WithContext( "CreateBuffer" );
        LUMEN_CHECK_EQ( static_cast<UInt16>( Context.Code ), static_cast<UInt16>( ERHIErrorCode::OutOfMemory ) );
        LUMEN_CHECK_EQ( static_cast<UInt16>( Context.Condition ), static_cast<UInt16>( ERHIErrorCondition::Memory ) );
        LUMEN_CHECK_EQ( Context.NativeCode.ValueOr( 0 ), 42 );
        LUMEN_CHECK_TRUE( Context.Message.find( "CreateBuffer" ) != FString::npos );

        const TExpected<UInt32, FRHIError> Failure{ TUnexpected<FRHIError>( Error ) };
        const UInt32 Recovered = Failure.ValueOr( 7U );
        LUMEN_CHECK_EQ( Recovered, 7U );
    }

} // namespace RHI

} // namespace Lumen
