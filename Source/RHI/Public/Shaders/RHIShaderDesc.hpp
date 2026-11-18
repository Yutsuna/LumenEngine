/**
 * @file RHIShaderDesc.hpp
 * @brief Defines shader module creation parameters.
 */
#pragma once
#include <Memory/Span.hpp>
#include <RHIExport.hpp>
#include <Shaders/RHIShaderFormat.hpp>
#include <Shaders/RHIShaderReflection.hpp>
#include <String/String.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIShaderStage.hpp>
namespace Lumen

{
/**
 * @struct FRHIShaderDesc
 * @brief Non-owning bytecode input and owned entry-point text.
 */
struct RHI_EXPORT FRHIShaderDesc
{
    TSpan<const Byte> Bytecode;
    ERHIShaderStage Stage   = ERHIShaderStage::None;
    FString EntryPoint      = "Main";
    ERHIShaderFormat Format = ERHIShaderFormat::Null;
    FRHIShaderReflection Reflection;
    /** @return Success for a single stage, nonempty entry, and valid reflection. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
