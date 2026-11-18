/**
 * @file RHIDrawArguments.cpp
 * @brief Implements draw and dispatch argument validation.
 */

#include <Commands/RHIDrawArguments.hpp>
#include <Utilities/RHICheckedMath.hpp>
namespace
{
Lumen::TExpected<void, Lumen::FRHIError> InvalidArguments ( Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected<Lumen::FRHIError>( Lumen::MakeRHIInvalidArgument( Lumen::ERHIErrorCondition::Command, InMessage ) );
}
} // namespace
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIDrawArguments::Validate () const noexcept
{
    if ( VertexCount == 0U or InstanceCount == 0U )
    {
        return InvalidArguments( "Draw counts must be nonzero" );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIDrawIndexedArguments::Validate () const noexcept
{
    if ( IndexCount == 0U or InstanceCount == 0U )
    {
        return InvalidArguments( "Indexed draw counts must be nonzero" );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIDispatchArguments::Validate ( const UInt32 InMaximumPerDimension ) const noexcept
{
    if ( GroupCountX == 0U or GroupCountY == 0U or GroupCountZ == 0U or GroupCountX > InMaximumPerDimension or
         GroupCountY > InMaximumPerDimension or GroupCountZ > InMaximumPerDimension )
    {
        return InvalidArguments( "Dispatch group counts are zero or exceed device limits" );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIIndirectArguments::Validate ( const UInt64 InBufferSize,
                                                                                  const UInt32 InMinimumStride ) const noexcept
{
    if ( Count == 0U or InMinimumStride == 0U or Stride < InMinimumStride or Stride % 4U != 0U or Offset % 4ULL != 0ULL )
    {
        return InvalidArguments( "Indirect arguments have invalid count, stride, or alignment" );
    }
    const TOptional<UInt64> CommandsSize = RHICheckedMultiply( static_cast<UInt64>( Count - 1U ), static_cast<UInt64>( Stride ) );
    const TOptional<UInt64> RequiredSize =
        CommandsSize ? RHICheckedAdd( *CommandsSize, static_cast<UInt64>( InMinimumStride ) ) : TOptional<UInt64>{};
    const TOptional<UInt64> End = RequiredSize ? RHICheckedAdd( Offset, *RequiredSize ) : TOptional<UInt64>{};
    if ( not End or *End > InBufferSize )
    {
        return InvalidArguments( "Indirect arguments exceed the buffer" );
    }
    return {};
}
