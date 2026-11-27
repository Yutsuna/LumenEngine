/**
 * @file RHICapabilities.hpp
 * @brief Defines optional RHI feature negotiation.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIShaderFormat.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHICapabilityFlags
     * @brief Optional features a backend may advertise.
     */
    enum class ERHICapabilityFlags : UInt64 /** NOLINT */
    {
        None                      = 0ULL,
        DynamicRendering          = 1ULL << 0U,
        TimelineSemaphores        = 1ULL << 1U,
        BindlessResources         = 1ULL << 2U,
        IndirectDraw              = 1ULL << 3U,
        MultiDrawIndirect         = 1ULL << 4U,
        TimestampQueries          = 1ULL << 5U,
        OcclusionQueries          = 1ULL << 6U,
        PipelineStatistics        = 1ULL << 7U,
        MeshShaders               = 1ULL << 8U,
        RayTracing                = 1ULL << 9U,
        VariableRateShading       = 1ULL << 10U,
        ConservativeRasterization = 1ULL << 11U,
        DepthClamp                = 1ULL << 12U,
        AnisotropicFiltering      = 1ULL << 13U,
        AsyncCompute              = 1ULL << 14U,
        Presentation              = 1ULL << 15U,
        GeometryShaders           = 1ULL << 16U,
        Tessellation              = 1ULL << 17U,
    };

    LUMEN_ENUM_FLAGS( ERHICapabilityFlags );

    /**
     * @struct FRHICapabilities
     * @brief Immutable feature set advertised or requested by a device.
     */
    struct RHI_EXPORT FRHICapabilities
    {
        ERHICapabilityFlags Features = ERHICapabilityFlags::None;
        UInt32 ShaderModelMajor      = 0U;
        UInt32 ShaderModelMinor      = 0U;
        TVector<ERHIShaderFormat> ShaderFormats;

        /** @return True when every bit in InRequired is present. */
        [[nodiscard]] Bool Supports ( ERHICapabilityFlags InRequired ) const noexcept;
        /** @return True when the advertised formats contain InFormat. */
        [[nodiscard]] Bool Supports ( ERHIShaderFormat InFormat ) const noexcept;
        /** @return Success, or the first unsupported feature group. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
