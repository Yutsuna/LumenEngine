/**
 * @file RHIShaderPackageTests.cpp
 * @brief Tests owned shader package entries and lookup.
 */

#include <Shaders/RHIShaderPackage.hpp>
#include <Tests/UnitTest.hpp>

/**
 * Public
 */

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIShaderPackage, OwnsEntriesAndFindsStageAndEntryPoint )
    {
        FRHIShaderPackageEntry Entry;
        Entry.Bytecode   = { 1U, 2U, 3U };
        Entry.Stage      = ERHIShaderStage::Pixel;
        Entry.EntryPoint = "Shade";
        Entry.Format     = ERHIShaderFormat::PortableBytecode;
        Entry.Reflection.Bindings.push_back( { .Slot = 2U, .Stages = ERHIShaderStage::Pixel } );
        const FRHIShaderDesc Desc = Entry.GetDesc();
        LUMEN_CHECK_EQ( Desc.Bytecode.data(), Entry.Bytecode.data() );
        LUMEN_CHECK_EQ( Desc.Stage, Entry.Stage );
        FRHIShaderPackage Package;
        LUMEN_CHECK_TRUE( Package.AddEntry( std::move( Entry ) ).HasValue() );
        TOptional<FRHIShaderDesc> Found = Package.Find( ERHIShaderStage::Pixel, "Shade" );
        LUMEN_CHECK_TRUE( Found.HasValue() );
        LUMEN_CHECK_EQ( Found->Bytecode.size(), 3U );
        LUMEN_CHECK_EQ( Found->Reflection.Bindings[0].Slot, 2U );
        LUMEN_CHECK_FALSE( Package.Find( ERHIShaderStage::Vertex, "Shade" ).HasValue() );
        LUMEN_CHECK_FALSE( Package.Find( ERHIShaderStage::Pixel, "Other" ).HasValue() );
    }
    LUMEN_TEST( RHIShaderPackage, RejectsInvalidEntryWithoutAppending )
    {
        FRHIShaderPackage Package;
        FRHIShaderPackageEntry Entry;
        LUMEN_CHECK_HAS_ERROR( Package.AddEntry( std::move( Entry ) ) );
        LUMEN_CHECK_EQ( Package.GetEntries().size(), 0U );
    }

} // namespace
} // namespace Lumen
