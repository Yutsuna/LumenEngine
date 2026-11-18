/**
 * @file RHILog.hpp
 * @brief Declares private adapters from RHI diagnostics to the Core logger.
 */
#pragma once

#include <Logging/Logger.hpp>
#include <RHI/Debug/RHIDebugMessage.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /** @return Shared RHI log category. */
    [[nodiscard]] RHI_EXPORT const FLogCategory &GetRHILogCategory () noexcept;
    /** @return Core verbosity corresponding to an RHI severity. */
    [[nodiscard]] RHI_EXPORT ELogVerbosity::Type ToRHILogVerbosity ( ERHIDebugSeverity InSeverity ) noexcept;
    /** @brief Emits one message to the Core logger without forwarding it to another sink. */
    RHI_EXPORT void LogRHIDebugMessage ( const FRHIDebugMessage &InMessage ) noexcept;

} // namespace RHI

} // namespace Lumen
