/**
 * @file RHIAcquiredImage.hpp
 * @brief Defines one acquired swapchain image and its synchronization primitives.
 */

#pragma once

#include <Memory/SharedPtr.hpp>
#include <Presentation/RHISwapchainCapabilities.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

class IRHITexture;
class IRHISemaphore;

/**
 * @struct FRHIAcquiredImage
 * @brief A frame token that must be presented to the originating swapchain.
 */
struct RHI_EXPORT FRHIAcquiredImage
{
    UInt32 ImageIndex = 0U;
    TSharedPtr<IRHITexture> Image;
    /** Signaled when the acquired image is writable by the renderer. */
    TSharedPtr<IRHISemaphore> AcquireSemaphore;
    /** Signaled after presentation so the next acquire can reuse this frame slot. */
    TSharedPtr<IRHISemaphore> PresentSemaphore;
    Bool bSuboptimal = false;
};

} // namespace Lumen
