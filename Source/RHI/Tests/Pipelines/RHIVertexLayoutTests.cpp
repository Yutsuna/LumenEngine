/**
 * @file RHIVertexLayoutTests.cpp
 * @brief Tests vertex layout validation and content hashing.
 */

#include <cstring>

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

    LUMEN_TEST( RHIVertexLayout, HashIgnoresIndeterminateStructPadding )
    {
        /** NOTE: Cast through void* so memset is applied to the object representation of these trivially copyable structs. */
        const auto PrimeBytes = [] ( void *InObject, const USize InSize, const Byte InFill ) noexcept
        {
            std::memset( InObject, static_cast<Int32>( InFill ), InSize );
        };

        /** NOTE: Prime the whole object representation, then restore declared fields so only padding differs. */
        FRHIVertexAttribute AttrA;
        FRHIVertexAttribute AttrB;
        PrimeBytes( &AttrA, sizeof( AttrA ), 0xFFU );
        PrimeBytes( &AttrB, sizeof( AttrB ), 0x00U );
        AttrA.Location   = 0U;
        AttrA.BufferSlot = 0U;
        AttrA.Format     = ERHIFormat::RGB32Float;
        AttrA.Offset     = 0U;
        AttrB.Location   = AttrA.Location;
        AttrB.BufferSlot = AttrA.BufferSlot;
        AttrB.Format     = AttrA.Format;
        AttrB.Offset     = AttrA.Offset;

        FRHIVertexBufferLayout BufferA;
        FRHIVertexBufferLayout BufferB;
        PrimeBytes( &BufferA, sizeof( BufferA ), 0xFFU );
        PrimeBytes( &BufferB, sizeof( BufferB ), 0x00U );
        BufferA.Slot   = 0U;
        BufferA.Stride = 24U;
        BufferA.Rate   = ERHIVertexInputRate::Vertex;
        BufferB.Slot   = BufferA.Slot;
        BufferB.Stride = BufferA.Stride;
        BufferB.Rate   = BufferA.Rate;

        FRHIVertexLayout LayoutA;
        LayoutA.Buffers.push_back( BufferA );
        LayoutA.Attributes.push_back( AttrA );
        FRHIVertexLayout LayoutB;
        LayoutB.Buffers.push_back( BufferB );
        LayoutB.Attributes.push_back( AttrB );

        /** NOTE: Re-prime the stored elements in place so padding differences cannot be lost by a memberwise copy. */
        PrimeBytes( LayoutA.Buffers.data(), sizeof( FRHIVertexBufferLayout ), 0xFFU );
        PrimeBytes( LayoutB.Buffers.data(), sizeof( FRHIVertexBufferLayout ), 0x00U );
        PrimeBytes( LayoutA.Attributes.data(), sizeof( FRHIVertexAttribute ), 0xFFU );
        PrimeBytes( LayoutB.Attributes.data(), sizeof( FRHIVertexAttribute ), 0x00U );
        /** NOTE: Restore every declared field after the byte-wise priming; only padding must differ. */
        for ( FRHIVertexLayout *Layout : { &LayoutA, &LayoutB } )
        {
            Layout->Buffers[0].Slot          = 0U;
            Layout->Buffers[0].Stride        = 24U;
            Layout->Buffers[0].Rate          = ERHIVertexInputRate::Vertex;
            Layout->Attributes[0].Location   = 0U;
            Layout->Attributes[0].BufferSlot = 0U;
            Layout->Attributes[0].Format     = ERHIFormat::RGB32Float;
            Layout->Attributes[0].Offset     = 0U;
        }

        LUMEN_CHECK_EQ( LayoutA.GetHash(), LayoutB.GetHash() );

        /** NOTE: Changing a real field must change the hash, so the equality check cannot pass vacuously. */
        LayoutB.Attributes[0].Offset = 8U;
        LUMEN_CHECK_NOT_EQ( LayoutA.GetHash(), LayoutB.GetHash() );
        LayoutB.Attributes[0].Offset = AttrA.Offset;
        LayoutB.Buffers[0].Stride    = 32U;
        LUMEN_CHECK_NOT_EQ( LayoutA.GetHash(), LayoutB.GetHash() );
    }

} // namespace
} // namespace Lumen
