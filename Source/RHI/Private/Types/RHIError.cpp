/**
 * @file RHIError.cpp
 * @brief Implements structured RHI error values.
 */

#include <Types/RHIError.hpp>

Lumen::FRHIError::FRHIError ( const ERHIErrorCode InCode,
                              const ERHIErrorCondition InCondition,
                              const FStringView InMessage,
                              const TOptional<Int64> InNativeCode ) noexcept
    : Code( InCode ), Condition( InCondition ), Message( InMessage ), NativeCode( InNativeCode )
{
}

Lumen::FRHIError Lumen::FRHIError::WithContext ( const FStringView InOperation ) const noexcept
{
    FString Context( InOperation );
    Context += ": ";
    Context += Message;
    return FRHIError( Code, Condition, Context, NativeCode );
}
