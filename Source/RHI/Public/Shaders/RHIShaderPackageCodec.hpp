/**
 * @file RHIShaderPackageCodec.hpp
 * @brief Declares versioned shader package serialization.
 */
#pragma once
#include <RHIExport.hpp>
#include <Shaders/RHIShaderPackage.hpp>
namespace Lumen
{
/** @brief Serializes a package into a self-contained byte vector. */
[[nodiscard]] RHI_EXPORT TExpected<TVector<Byte>, FRHIError>
EncodeRHIShaderPackage ( const FRHIShaderPackage &InPackage ) noexcept;
/** @brief Decodes and validates a shader package from bytes. */
[[nodiscard]] RHI_EXPORT TExpected<FRHIShaderPackage, FRHIError> DecodeRHIShaderPackage ( TSpan<const Byte> InBytes ) noexcept;
} // namespace Lumen
