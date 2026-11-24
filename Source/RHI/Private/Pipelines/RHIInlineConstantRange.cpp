/**
 * @file RHIInlineConstantRange.cpp
 * @brief Implements inline constant range validation.
 */

#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIInlineConstantRange::Validate () const noexcept
{
    if ( Size == 0U or Stages == ERHIShaderStage::None or Offset % 4U != 0U or Size % 4U != 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid inline constant range" ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::RHI::ValidateRHIInlineConstantWrite ( const ERHIShaderStage InStages, const UInt32 InOffset, const USize InSize ) noexcept
{
    const UInt32 RawStages = static_cast<UInt32>( InStages );
    const UInt32 AllStages = static_cast<UInt32>( ERHIShaderStage::All );
    if ( RawStages == 0U or ( RawStages & ~AllStages ) != 0U or InOffset % 4U != 0U or InSize == 0U or InSize % 4U != 0U )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid inline constant stages, offset, or size" ) );
    }
    const TOptional<UInt32> Size = RHICheckedCast<UInt32>( InSize );
    if ( not Size or not RHICheckedAdd( InOffset, *Size ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Inline constant range overflows" ) );
    }
    return {};
}
