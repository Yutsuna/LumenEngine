/**
 * @file RHIVertexLayoutTests.cpp
 * @brief Tests vertex layout validation and content hashing.
 */

#include <Pipelines/RHIVertexLayout.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    namespace
    {

        FRHIVertexLayout MakeValidLayout ()
        {
            FRHIVertexLayout Layout;
            Layout.Buffers.push_back( { .Slot = 0U, .Stride = 24U, .Rate = ERHIVertexInputRate::Vertex } );
            Layout.Attributes.push_back( { .Location = 0U, .BufferSlot = 0U, .Format = ERHIFormat::RGB32Float, .Offset = 0U } );
            Layout.Attributes.push_back( { .Location = 1U, .BufferSlot = 0U, .Format = ERHIFormat::RGB32Float, .Offset = 12U } );
            return Layout;
        }

    } // namespace

    LUMEN_TEST( RHIVertexLayout, RejectsMissingBuffersZeroStridesAndDuplicateLocations )
    {
        FRHIVertexLayout Layout = MakeValidLayout();
        LUMEN_CHECK_TRUE( Layout.Validate().HasValue() );
        Layout.Attributes[0].BufferSlot = 1U;
        LUMEN_CHECK_HAS_ERROR( Layout.Validate() );
        Layout.Attributes[0].BufferSlot = 0U;
        Layout.Attributes[1].Location   = 0U;
        LUMEN_CHECK_HAS_ERROR( Layout.Validate() );
        Layout.Attributes[1].Location = 1U;
        Layout.Buffers[0].Stride      = 0U;
        LUMEN_CHECK_HAS_ERROR( Layout.Validate() );
    }

    LUMEN_TEST( RHIVertexLayout, RejectsDuplicateBufferSlotsAndUnknownFormats )
    {
        FRHIVertexLayout Layout = MakeValidLayout();
        Layout.Buffers.push_back( { .Slot = 0U, .Stride = 16U, .Rate = ERHIVertexInputRate::Instance } );
        LUMEN_CHECK_HAS_ERROR( Layout.Validate() );
        Layout                      = MakeValidLayout();
        Layout.Attributes[0].Format = ERHIFormat::Unknown;
        LUMEN_CHECK_HAS_ERROR( Layout.Validate() );
        Layout.Attributes[0].Format = ERHIFormat::Count;
        LUMEN_CHECK_HAS_ERROR( Layout.Validate() );
    }

    LUMEN_TEST( RHIVertexLayout, HashIncludesAllOrderedContent )
    {
        FRHIVertexLayout Layout = MakeValidLayout();
        const UInt64 Hash       = Layout.GetHash();
        LUMEN_CHECK_NOT_EQ( Hash, 0ULL );
        Layout.Attributes[1].Offset = 8U;
        LUMEN_CHECK_NOT_EQ( Layout.GetHash(), Hash );
    }

} // namespace
} // namespace Lumen
