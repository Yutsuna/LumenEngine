/**
 * @file RHIError.cpp
 * @brief Implements structured RHI error values.
 */

#include <RHI/Types/RHIError.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIError::FRHIError ( const ERHIErrorCode InCode,
                                   const ERHIErrorCondition InCondition,
                                   const FStringView InMessage,
                                   const TOptional<Int64> InNativeCode ) noexcept
    : Code( InCode ), Condition( InCondition ), Message( InMessage ), NativeCode( InNativeCode )
{
    /* */
}

Lumen::RHI::FRHIError Lumen::RHI::FRHIError::WithContext ( const FStringView InOperation ) const noexcept
{
    FString Context( InOperation );
    Context += ": ";
    Context += Message;
    return { Code, Condition, Context, NativeCode };
}
