/**
 * @file RHIDrawArguments.cpp
 * @brief Implements draw and dispatch argument validation.
 */

#include <RHI/Commands/RHIDrawArguments.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>
namespace
{
Lumen::RHI::FRHIExpected InvalidArguments ( Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected<Lumen::RHI::FRHIError>(
        Lumen::RHI::MakeRHIInvalidArgument( Lumen::RHI::ERHIErrorCondition::Command, InMessage ) );
}
} // namespace
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDrawArguments::Validate () const noexcept
{
    if ( VertexCount == 0U or InstanceCount == 0U )
    {
        return InvalidArguments( "Draw counts must be nonzero" );
    }
    return {};
}
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDrawIndexedArguments::Validate () const noexcept
{
    if ( IndexCount == 0U or InstanceCount == 0U )
    {
        return InvalidArguments( "Indexed draw counts must be nonzero" );
    }
    return {};
}
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDispatchArguments::Validate ( const UInt32 InMaximumPerDimension ) const noexcept
{
    if ( GroupCountX == 0U or GroupCountY == 0U or GroupCountZ == 0U or GroupCountX > InMaximumPerDimension or
         GroupCountY > InMaximumPerDimension or GroupCountZ > InMaximumPerDimension )
    {
        return InvalidArguments( "Dispatch group counts are zero or exceed device limits" );
    }
    return {};
}
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIIndirectArguments::Validate ( const UInt64 InBufferSize,
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
