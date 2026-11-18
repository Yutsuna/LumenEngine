/**
 * @file RHIBlendStateTests.cpp
 * @brief Tests blend attachment validation.
 */

#include <RHI/Pipelines/RHIBlendState.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIBlendState, AcceptsDefaultAndEnabledAttachment )
        {
            FRHIBlendState State;
            LUMEN_CHECK_TRUE( State.Validate().HasValue() );
            State.Attachments.push_back( { .bEnabled         = true,
                                           .SourceColor      = ERHIBlendFactor::SourceAlpha,
                                           .DestinationColor = ERHIBlendFactor::InverseSourceAlpha,
                                           .ColorOp          = ERHIBlendOp::Add,
                                           .SourceAlpha      = ERHIBlendFactor::One,
                                           .DestinationAlpha = ERHIBlendFactor::Zero,
                                           .AlphaOp          = ERHIBlendOp::Add,
                                           .WriteMask        = 0xFU } );
            LUMEN_CHECK_TRUE( State.Validate().HasValue() );
        }

        LUMEN_TEST( RHIBlendState, RejectsInvalidMaskOperationsAndFactors )
        {
            FRHIBlendState State;
            State.Attachments.push_back( {} );
            State.Attachments[0].WriteMask = 0x10U;
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
            State.Attachments[0].WriteMask = 0xFU;
            State.Attachments[0].ColorOp   = static_cast<ERHIBlendOp>( 255U );
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
            State.Attachments[0].ColorOp = ERHIBlendOp::Add;
            State.Attachments[0].AlphaOp = static_cast<ERHIBlendOp>( 5U );
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
            State.Attachments[0].AlphaOp     = ERHIBlendOp::Add;
            State.Attachments[0].SourceColor = static_cast<ERHIBlendFactor>( 6U );
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
            State.Attachments[0].SourceColor      = ERHIBlendFactor::One;
            State.Attachments[0].DestinationAlpha = static_cast<ERHIBlendFactor>( 255U );
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
