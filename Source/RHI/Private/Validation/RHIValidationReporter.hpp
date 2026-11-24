/**
 * @file RHIValidationReporter.hpp
 * @brief Declares validation diagnostic filtering and structured rejection.
 */
#pragma once

#include <RHI/Validation/RHIValidationDesc.hpp>

#include <HAL/PlatformMutex.hpp>
#include <Templates/Map.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIObject;

    /** @enum ERHIValidationMessageCode @brief Stable private diagnostic codes used by validation tests and sinks. */
    enum class ERHIValidationMessageCode : UInt32
    {
        InvalidDescriptor = 1U,
        UnknownObject,
        ForeignObject,
        ObjectLeak,
        InvalidResourceTransition,
        InvalidBinding,
        InvalidCommandState,
        InvalidQueueSubmission,
        InvalidPresentation,
        InvalidTransientBatch,
        InvalidDefragPlan
    };

    /**
     * @class FRHIValidationReporter
     * @brief Routes diagnostics without introducing a logger dependency.
     */
    class RHI_EXPORT FRHIValidationReporter final
    {
    public:

        explicit FRHIValidationReporter ( const FRHIValidationDesc &InDesc ) noexcept;

        /** @brief Emits a validation message when it passes the configured severity and deduplication policy. */
        void Report ( ERHIValidationMessageCode InCode,
                      FStringView InMessage,
                      const IRHIObject *InObject   = nullptr,
                      ERHIDebugSeverity InSeverity = ERHIDebugSeverity::Error ) const noexcept;
        /** @return A ValidationFailed error and emits its corresponding diagnostic. */
        [[nodiscard]] FRHIExpected Reject ( ERHIValidationMessageCode InCode,
                                            ERHIErrorCondition InCondition,
                                            FStringView InMessage,
                                            const IRHIObject *InObject = nullptr ) const noexcept;
        /** @return The immutable diagnostic policy. */
        [[nodiscard]] const FRHIValidationDesc &GetDesc () const noexcept;

    private:

        FRHIValidationDesc Desc;
        mutable TMap<UInt64, FString> SeenMessages;
        mutable HAL::FPlatformMutex Mutex;
    };

} // namespace RHI

} // namespace Lumen
