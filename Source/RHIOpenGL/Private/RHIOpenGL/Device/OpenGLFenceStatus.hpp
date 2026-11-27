/**
 * @file OpenGLFenceStatus.hpp
 * @brief Pure classification of glClientWaitSync results for the GL executor.
 */

#pragma once

#include <RHIOpenGL/Loader/GLSpec.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /** @brief Outcome of a non-blocking glClientWaitSync poll. */
    enum class EFenceWaitStatus : UInt8
    {
        Timeout,
        Complete,
        Failed
    };

    /**
     * @brief Maps a native wait result onto a stable outcome.
     * @details Only the two positive results count as completion; every other value,
     *          including unknown driver results, fails closed so a lost context can
     *          never be published as successful work.
     */
    [[nodiscard]] constexpr EFenceWaitStatus ClassifyFenceWaitStatus ( const GLenum InStatus ) noexcept
    {
        switch ( InStatus )
        {
        case GlSyncConditionSatisfied:
        case GlSyncAlreadySignaled:
            return EFenceWaitStatus::Complete;
        case GlSyncTimeoutExpired:
            return EFenceWaitStatus::Timeout;
        default:
            return EFenceWaitStatus::Failed;
        }
    }

} // namespace RHIOpenGL

} // namespace Lumen
