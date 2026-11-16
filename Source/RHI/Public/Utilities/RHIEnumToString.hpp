/**
 * @file RHIEnumToString.hpp
 * @brief Declares stable textual names for RHI enums.
 */
#pragma once

#include <Caches/RHIObjectCache.hpp>
#include <Debug/RHIDebugMessage.hpp>
#include <Presentation/RHIColorSpace.hpp>
#include <Presentation/RHIPresentMode.hpp>
#include <Presentation/RHISurfaceDesc.hpp>
#include <Queries/RHIQueryType.hpp>
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
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIPresentMode InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIColorSpace InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHISurfaceType InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIQueryType InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIDebugSeverity InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIDebugCategory InValue ) noexcept;
[[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHICacheEvictionPolicy InValue ) noexcept;

} // namespace Lumen
