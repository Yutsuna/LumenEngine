/**
 * @file IRHISwapchain.hpp
 * @brief Declares swapchain image acquisition, presentation, and resizing.
 */
#pragma once

#include <Presentation/RHIAcquiredImage.hpp>
#include <Presentation/RHISwapchainDesc.hpp>
#include <Types/IRHIObject.hpp>

namespace Lumen
{

class IRHIQueue;

/**
 * @class IRHISwapchain
 * @brief Backend-independent bounded image ring.
 */
class IRHISwapchain : public IRHIObject
{
public:

    [[nodiscard]] virtual const FRHISwapchainDesc &GetDesc () const noexcept                                       = 0;
    [[nodiscard]] virtual const FRHISwapchainCapabilities &GetCapabilities () const noexcept                       = 0;
    [[nodiscard]] virtual UInt32 GetImageCount () const noexcept                                                   = 0;
    [[nodiscard]] virtual TExpected<TSharedRef<IRHITexture>, FRHIError> GetImage ( UInt32 InIndex ) const noexcept = 0;
    /** @brief Acquires the next free image; OutOfDate requires recreation. */
    [[nodiscard]] virtual TExpected<FRHIAcquiredImage, FRHIError> AcquireNextImage () noexcept = 0;
    /** @brief Presents an acquired image after waiting on the optional binary render-complete semaphore. */
    [[nodiscard]] virtual TExpected<void, FRHIError>
    Present ( IRHIQueue &InQueue, const FRHIAcquiredImage &InImage, IRHISemaphore *InRenderComplete ) noexcept = 0;
    /** @brief Recreates the image ring without invalidating already acquired images. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Resize ( const FRHIExtent3D &InExtent ) noexcept = 0;
};

} // namespace Lumen
