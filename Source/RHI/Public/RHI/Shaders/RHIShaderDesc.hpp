/**
 * @file RHIShaderDesc.hpp
 * @brief Defines shader module creation parameters.
 */
#pragma once
#include <Memory/Span.hpp>
#include <RHI/Shaders/RHIShaderReflection.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIShaderFormat.hpp>
#include <RHI/Types/RHIShaderStage.hpp>
#include <RHIExport.hpp>
#include <String/String.hpp>
namespace Lumen
{

namespace RHI
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
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };
} // namespace RHI

} // namespace Lumen
