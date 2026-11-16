/**
 * @file RHIBindingLayoutDescTests.cpp
 * @brief Tests binding layout validation and canonical hashing.
 */

#include <Bindings/RHIBindingLayoutDesc.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIBindingLayoutDesc, RejectsInvalidEntriesAndDuplicateSlots )
    {
        FRHIBindingLayoutDesc Desc;
        Desc.Entries.push_back( { .Slot = 0U, .Stages = ERHIShaderStage::Vertex } );
        Desc.Entries.push_back( { .Slot = 2U, .Stages = ERHIShaderStage::Pixel } );
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.Entries[1].Count = 0U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Entries[1].Count  = 1U;
        Desc.Entries[1].Stages = ERHIShaderStage::None;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Entries[1].Stages = ERHIShaderStage::Pixel;
        Desc.Entries[1].Slot   = 0U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }
    LUMEN_TEST( RHIBindingLayoutDesc, HashIgnoresOrderButIncludesContent )
    {
        const FRHIBindingLayoutEntry First{
            .Slot = 1U, .Type = ERHIBindingType::Texture, .Count = 2U, .Stages = ERHIShaderStage::Pixel, .bBindless = true };
        const FRHIBindingLayoutEntry Second{ .Slot      = 4U,
                                             .Type      = ERHIBindingType::Sampler,
                                             .Count     = 1U,
                                             .Stages    = ERHIShaderStage::AllGraphics,
                                             .bBindless = false };
        FRHIBindingLayoutDesc Ordered;
        Ordered.Entries = { First, Second };
        FRHIBindingLayoutDesc Reversed;
        Reversed.Entries = { Second, First };
        LUMEN_CHECK_EQ( Ordered.GetHash(), Reversed.GetHash() );
        Reversed.Entries[0].Count = 3U;
        LUMEN_CHECK_NOT_EQ( Ordered.GetHash(), Reversed.GetHash() );
    }

} // namespace
} // namespace Lumen
