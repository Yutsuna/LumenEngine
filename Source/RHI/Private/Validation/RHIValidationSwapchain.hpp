/**
 * @file RHIValidationSwapchain.hpp
 * @brief Declares swapchain decoration and acquired-image token tracking.
 */
#pragma once

#include "RHIValidationResources.hpp"
#include <Presentation/IRHISwapchain.hpp>

namespace Lumen
{

/**
 * @class FRHIValidationSwapchain
 * @brief Validates acquisition, presentation, and resize scopes.
 */
class FRHIValidationSwapchain final : public IRHISwapchain, public IRHIValidationObject
{
public:

    FRHIValidationSwapchain ( TSharedRef<IRHISwapchain> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationSwapchain () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHISwapchainDesc &GetDesc () const noexcept override;
    [[nodiscard]] const FRHISwapchainCapabilities &GetCapabilities () const noexcept override;
    [[nodiscard]] UInt32 GetImageCount () const noexcept override;
    [[nodiscard]] TExpected<TSharedRef<IRHITexture>, FRHIError> GetImage ( UInt32 InIndex ) const noexcept override;
    [[nodiscard]] TExpected<FRHIAcquiredImage, FRHIError> AcquireNextImage () noexcept override;
    [[nodiscard]] TExpected<void, FRHIError>
    Present ( IRHIQueue &InQueue, const FRHIAcquiredImage &InImage, IRHISemaphore *InRenderComplete ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Resize ( const FRHIExtent3D &InExtent ) noexcept override;

private:

    void RebuildImages () noexcept;

private:

    TSharedRef<IRHISwapchain> Underlying;
    TSharedPtr<FRHIValidationState> State;
    TVector<TSharedRef<IRHITexture>> Images;
    TVector<Bool> Acquired;
};

} // namespace Lumen
