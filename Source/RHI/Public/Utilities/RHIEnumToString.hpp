/**
 * @file RHIEnumToString.hpp
 * @brief Declares stable textual names for RHI enums.
 */
#pragma once

#include <RHIExport.hpp>
#include <String/StringView.hpp>
#include <Types/RHICapabilities.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIFormat.hpp>
#include <Types/RHIQueueType.hpp>
#include <Types/RHIResourceState.hpp>
#include <Types/RHISampleCount.hpp>
#include <Types/RHIShaderStage.hpp>

namespace Lumen
{

/** @return Stable enum name or "Unknown" for an unrecognized value. */
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIFormat InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIErrorCode InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIErrorCondition InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIQueueType InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIResourceState InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHISampleCount InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIShaderStage InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHICapabilityFlags InValue ) noexcept;

} // namespace Lumen
