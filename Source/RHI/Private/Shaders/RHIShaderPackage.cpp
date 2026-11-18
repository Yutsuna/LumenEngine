/**
 * @file RHIShaderPackage.cpp
 * @brief Implements owned shader package entries.
 */

#include <RHI/Shaders/RHIShaderPackage.hpp>
Lumen::RHI::FRHIShaderDesc Lumen::RHI::FRHIShaderPackageEntry::GetDesc () const noexcept
{
    return FRHIShaderDesc{ TSpan<const Byte>( Bytecode.data(), Bytecode.size() ), Stage, EntryPoint, Format, Reflection };
}
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIShaderPackage::AddEntry ( FRHIShaderPackageEntry InEntry ) noexcept
{
    if ( const TExpected<void, FRHIError> Valid = InEntry.GetDesc().Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
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
