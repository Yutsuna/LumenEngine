/**
 * @file RHIBufferPool.hpp
 * @brief Declares the device-backed buffer pool implementation.
 */
#pragma once
#include "../Types/RHIObjectForward.hpp"
#include <Device/IRHIDevice.hpp>
#include <Memory/IRHIBufferPool.hpp>
#include <Resources/RHIBufferDesc.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHITimeline.hpp>
namespace Lumen
{

/**
 * @class FRHIBufferPool
 * @brief Creates buffers and retains released entries until safe reuse.
 */
class RHI_EXPORT FRHIBufferPool final : public IRHIBufferPool
{
public:

    LUMEN_RHI_OBJECT_DECLARATIONS;
    /** @brief Uses InDevice for creation and InId for identity. */
    FRHIBufferPool ( IRHIDevice &InDevice, FRHIObjectId InId ) noexcept;
    TExpected<TSharedRef<IRHIBuffer>, FRHIError> Acquire ( const FRHIBufferDesc &InDesc ) noexcept override;
    TExpected<void, FRHIError> Release ( TSharedRef<IRHIBuffer> InBuffer, FRHITimelineValue InCompletion ) noexcept override;
    /** @brief Publishes the current GPU completion value so finished entries become reusable. */
    void AdvanceCompleted ( FRHITimelineValue InCompleted ) noexcept override;
    /** @return Number of released entries still waiting for their completion value. */
    [[nodiscard]] USize GetPendingCount () const noexcept override;

private:

    FRHIObjectBase Identity;
    struct FEntry
    {
        TSharedRef<IRHIBuffer> Buffer;
        FRHITimelineValue Completion;
    };
    IRHIDevice *Device;
    TVector<FEntry> Released;
    FRHITimelineValue CurrentCompleted;
};
} // namespace Lumen
