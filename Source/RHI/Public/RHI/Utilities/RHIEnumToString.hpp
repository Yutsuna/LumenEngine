/**
 * @file RHIEnumToString.hpp
 * @brief Declares stable textual names for RHI enums.
 */

#pragma once

#include <RHI/Caches/RHIObjectCache.hpp>
#include <RHI/Debug/RHIDebugMessage.hpp>
#include <RHI/Presentation/RHIColorSpace.hpp>
#include <RHI/Presentation/RHIPresentMode.hpp>
#include <RHI/Presentation/RHISurfaceDesc.hpp>
#include <RHI/Queries/RHIQueryType.hpp>
#include <RHI/Types/RHICapabilities.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHI/Types/RHIResourceState.hpp>
#include <RHI/Types/RHISampleCount.hpp>
#include <RHI/Types/RHIShaderStage.hpp>
#include <RHIExport.hpp>
#include <String/StringView.hpp>

namespace Lumen
{

namespace RHI
{

    /** @brief Returns the stable name for Format, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIFormat InValue ) noexcept;
    /** @brief Returns the stable name for ErrorCode, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIErrorCode InValue ) noexcept;
    /** @brief Returns the stable name for ErrorCondition, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIErrorCondition InValue ) noexcept;
    /** @brief Returns the stable name for QueueType, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIQueueType InValue ) noexcept;
    /** @brief Returns the stable name for ResourceState, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIResourceState InValue ) noexcept;
    /** @brief Returns the stable name for SampleCount, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHISampleCount InValue ) noexcept;
    /** @brief Returns the stable name for ShaderStage, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIShaderStage InValue ) noexcept;
    /** @brief Returns the stable name for CapabilityFlags, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHICapabilityFlags InValue ) noexcept;
    /** @brief Returns the stable name for PresentMode, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIPresentMode InValue ) noexcept;
    /** @brief Returns the stable name for ColorSpace, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIColorSpace InValue ) noexcept;
    /** @brief Returns the stable name for SurfaceType, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHISurfaceType InValue ) noexcept;
    /** @brief Returns the stable name for QueryType, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIQueryType InValue ) noexcept;
    /** @brief Returns the stable name for DebugSeverity, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIDebugSeverity InValue ) noexcept;
    /** @brief Returns the stable name for DebugCategory, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHIDebugCategory InValue ) noexcept;
    /** @brief Returns the stable name for CacheEvictionPolicy, or "Unknown" for an unrecognized value. */
    [[nodiscard]] RHI_EXPORT FStringView ToRHIString ( ERHICacheEvictionPolicy InValue ) noexcept;

} // namespace RHI

} // namespace Lumen
