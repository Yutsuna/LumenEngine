/**
 * @file RHIShaderPackage.hpp
 * @brief Declares owned shader modules grouped in a package.
 */
#pragma once
#include <RHIExport.hpp>
#include <Shaders/RHIShaderDesc.hpp>
#include <Templates/Optional.hpp>
namespace Lumen

{
/**
 * @struct FRHIShaderPackageEntry
 * @brief Owned bytecode and shader metadata.
 */
struct RHI_EXPORT FRHIShaderPackageEntry
{
    TVector<Byte> Bytecode;
    ERHIShaderStage Stage   = ERHIShaderStage::None;
    FString EntryPoint      = "Main";
    ERHIShaderFormat Format = ERHIShaderFormat::Null;
    FRHIShaderReflection Reflection;
    /** @return A non-owning descriptor valid while this entry stays alive. */
    [[nodiscard]] FRHIShaderDesc GetDesc () const noexcept;
};

/**
 * @class FRHIShaderPackage
 * @brief Versioned collection of owned shader entries.
 */
class RHI_EXPORT FRHIShaderPackage final
{
public:

    /** @brief Validates and appends an entry. */
    [[nodiscard]] TExpected<void, FRHIError> AddEntry ( FRHIShaderPackageEntry InEntry ) noexcept;
    /** @return First entry matching stage and entry point. */
    [[nodiscard]] TOptional<FRHIShaderDesc> Find ( ERHIShaderStage InStage, FStringView InEntryPoint ) const noexcept;
    /** @return All package entries. */
    [[nodiscard]] const TVector<FRHIShaderPackageEntry> &GetEntries () const noexcept
    {
        return Entries;
    }

private:

    TVector<FRHIShaderPackageEntry> Entries;
};
} // namespace Lumen
