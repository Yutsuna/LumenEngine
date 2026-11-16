/**
 * @file RecordingDebugSink.hpp
 * @brief Declares a thread-safe diagnostic sink used by tests.
 */
#pragma once

#include <Debug/IRHIDebugSink.hpp>
#include <HAL/PlatformMutex.hpp>

namespace Lumen
{

namespace Tests
{

    /** @class FRecordingDebugSink @brief Retains an ordered copy of every emitted diagnostic. */
    class FRecordingDebugSink final : public IRHIDebugSink
    {
    public:

        void Emit ( const FRHIDebugMessage &InMessage ) noexcept override;
        [[nodiscard]] TVector<FRHIDebugMessage> GetMessages () const noexcept;
        [[nodiscard]] Bool ContainsCode ( UInt32 InCode ) const noexcept;
        [[nodiscard]] USize GetCount ( ERHIDebugSeverity InMinimumSeverity = ERHIDebugSeverity::Info ) const noexcept;
        void Clear () noexcept;

    private:

        TVector<FRHIDebugMessage> Messages;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace Tests

} // namespace Lumen
