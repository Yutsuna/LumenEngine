/**
 * @file RHIValidationDesc.hpp
 * @brief Defines validation coverage and diagnostic policy.
 */

#pragma once

#include <Core/EnumFlags.hpp>

#include <RHI/Debug/RHIDebugMessage.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIDebugSink;

    /**
     * @enum ERHIValidationLevel
     * @brief Enables no validation, essential checks, or every requested check.
     */
    enum class ERHIValidationLevel : UInt8
    {
        Disabled,
        Basic,
        Full,
        Count
    };

    /**
     * @enum ERHIValidationFlags
     * @brief Independent validation domains that can be enabled selectively.
     */
    enum class ERHIValidationFlags : UInt32 /** NOLINT */
    {
        None               = 0U,
        ObjectLifetime     = 1U << 0U,
        ResourceStates     = 1U << 1U,
        Bindings           = 1U << 2U,
        Commands           = 1U << 3U,
        Queues             = 1U << 4U,
        Presentation       = 1U << 5U,
        TransientResources = 1U << 6U,
        All                = ObjectLifetime | ResourceStates | Bindings | Commands | Queues | Presentation | TransientResources
    };
    LUMEN_ENUM_FLAGS( ERHIValidationFlags );

    /**
     * @struct FRHIValidationDesc
     * @brief Configuration retained by a validation decorator tree.
     */
    struct RHI_EXPORT FRHIValidationDesc
    {
        ERHIValidationLevel Level         = ERHIValidationLevel::Full;
        ERHIValidationFlags Flags         = ERHIValidationFlags::All;
        IRHIDebugSink *Sink               = nullptr;
        ERHIDebugSeverity MinimumSeverity = ERHIDebugSeverity::Warning;
        Bool bFatalOnError                = false;
        Bool bDeduplicateMessages         = true;

        /** @return Success when the level, flags, and severity are representable. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
        /** @return True when a domain is enabled by both the level and flags. */
        [[nodiscard]] Bool IsEnabled ( ERHIValidationFlags InDomain ) const noexcept;
    };

} // namespace RHI

} // namespace Lumen
