/**
 * @file IRHISwapchain.hpp
 * @brief Declares swapchain image acquisition, presentation, and resizing.
 */

#pragma once

#include <RHI/Presentation/RHIAcquiredImage.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIQueue;

    /**
     * @class IRHISwapchain
     * @brief Backend-independent bounded image ring.
     */
    class IRHISwapchain : public IRHIObject
    {
    public:

        /**
         * @brief Returns the immutable swapchain descriptor.
         * @return A reference owned by the swapchain.
         */
        [[nodiscard]] virtual const FRHISwapchainDesc &GetDesc () const noexcept = 0;
        /**
         * @brief Returns the capabilities negotiated at creation.
         * @return A reference owned by the swapchain.
         */
        [[nodiscard]] virtual const FRHISwapchainCapabilities &GetCapabilities () const noexcept = 0;
        /**
         * @brief Returns the number of images in the ring.
         * @return Image count.
         */
        [[nodiscard]] virtual UInt32 GetImageCount () const noexcept = 0;
        /**
         * @brief Returns the texture for a ring index.
         * @param InIndex Index in [0, GetImageCount).
         * @return The image texture, or InvalidArgument for an out-of-range index.
         */
        [[nodiscard]] virtual TExpected<TSharedRef<IRHITexture>, FRHIError> GetImage ( UInt32 InIndex ) const noexcept = 0;
        /** @brief Acquires the next free image; OutOfDate requires recreation. */
        [[nodiscard]] virtual TExpected<FRHIAcquiredImage, FRHIError> AcquireNextImage () noexcept = 0;
        /** @brief Presents an acquired image after waiting on the optional binary render-complete semaphore. */
        [[nodiscard]] virtual TExpected<void, FRHIError>
        Present ( IRHIQueue &InQueue, const FRHIAcquiredImage &InImage, IRHISemaphore *InRenderComplete ) noexcept = 0;
        /** @brief Recreates the image ring without invalidating already acquired images. */
        [[nodiscard]] virtual TExpected<void, FRHIError> Resize ( const FRHIExtent3D &InExtent ) noexcept = 0;
    };

} // namespace RHI

} // namespace Lumen
