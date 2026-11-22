/**
 * @file RHIShaderPackage.cpp
 * @brief Implements owned shader package entries.
 */

#include <RHI/Shaders/RHIShaderPackage.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIShaderDesc Lumen::RHI::FRHIShaderPackageEntry::GetDesc () const noexcept
{
    return FRHIShaderDesc{ .Bytecode   = TSpan<const Byte>( Bytecode.data(), Bytecode.size() ),
                           .Stage      = Stage,
                           .EntryPoint = EntryPoint,
                           .Format     = Format,
                           .Reflection = Reflection };
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIShaderPackage::AddEntry ( FRHIShaderPackageEntry InEntry ) noexcept
{
    LUMEN_EXPECT( _, InEntry.GetDesc().Validate() );
    Entries.push_back( std::move( InEntry ) );
    return {};
}

Lumen::TOptional<Lumen::RHI::FRHIShaderDesc> Lumen::RHI::FRHIShaderPackage::Find ( const ERHIShaderStage InStage,
                                                                                   const FStringView InEntryPoint ) const noexcept
{
    for ( const FRHIShaderPackageEntry &Entry : Entries )
    {
        if ( Entry.Stage == InStage and Entry.EntryPoint == InEntryPoint )
        {
            return TOptional<FRHIShaderDesc>( Entry.GetDesc() );
        }
    }
    return {};
}
