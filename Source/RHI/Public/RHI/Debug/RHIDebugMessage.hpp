/**
 * @file RHIDebugMessage.hpp
 * @brief Defines owned structured diagnostics emitted by RHI backends and validation.
 */
#pragma once

#include <RHI/Types/RHIIdentifiers.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHI/Types/RHITimeline.hpp>
#include <RHIExport.hpp>
#include <String/String.hpp>
#include <String/StringView.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIDebugSeverity
     * @brief Diagnostic urgency in ascending order.
     */
    enum class ERHIDebugSeverity : UInt8
    {
        Info,
        Warning,
        Error,
        Fatal,
        Count
    };

    /**
     * @enum ERHIDebugCategory
     * @brief Stable diagnostic source classification.
     */
    enum class ERHIDebugCategory : UInt8
    {
        General,
        Validation,
        Performance,
        Resource,
        Synchronization,
        Presentation,
        Pipeline,
        Count
    };

    /**
     * @struct FRHIDebugMessage
     * @brief Owned diagnostic text with object and submission context.
     */
    struct RHI_EXPORT FRHIDebugMessage
    {
        ERHIDebugSeverity Severity = ERHIDebugSeverity::Info;
        ERHIDebugCategory Category = ERHIDebugCategory::General;
        UInt32 Code                = 0U;
        FString Message;
        TVector<FRHIObjectId> ObjectIds;
        TOptional<ERHIQueueType> Queue;
        TOptional<FRHITimelineValue> Submission;

        /** @brief Creates an empty message with default severity and category. */
        FRHIDebugMessage () noexcept = default;
        FRHIDebugMessage ( ERHIDebugSeverity InSeverity,
                           ERHIDebugCategory InCategory,
                           UInt32 InCode,
                           FStringView InMessage ) noexcept;

        /** @brief Appends a valid object identifier to the diagnostic context. */
        void AddObject ( FRHIObjectId InId ) noexcept;
        /** @brief Associates the message with a queue submission timeline value. */
        void SetSubmissionContext ( ERHIQueueType InQueue, FRHITimelineValue InSubmission ) noexcept;
        /** @return True when this message has at least InSeverity. */
        [[nodiscard]] Bool IsAtLeast ( ERHIDebugSeverity InSeverity ) const noexcept;
    };

} // namespace RHI

} // namespace Lumen
