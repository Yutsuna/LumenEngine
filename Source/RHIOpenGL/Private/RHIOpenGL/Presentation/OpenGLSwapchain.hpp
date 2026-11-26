/**
 * @file OpenGLSwapchain.hpp
 * @brief Declares the bounded OpenGL image ring with explicit acquire/present state.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Presentation/IRHISurfaceBridge.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Presentation/RHIAcquiredImage.hpp>
#include <RHI/Presentation/RHISwapchainCapabilities.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Types/RHIError.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Synchronization/OpenGLSemaphore.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLSwapchain
     * @brief Windowed or Null offscreen swapchain backed by ordinary GL textures.
     * @details Slot state is guarded by a leaf mutex; native presentation is delegated
     *          to IRHISurfaceBridge. Mailbox is rejected because GL cannot promise it.
     */
    class FOpenGLSwapchain final : public RHI::IRHISwapchain, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLSwapchain>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const RHI::FRHISwapchainDesc &InDesc,
                 const RHI::FRHISwapchainCapabilities &InCapabilities,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLSwapchain ( TSharedRef<FOpenGLDeviceContext> InContext,
                           const RHI::FRHISwapchainDesc &InDesc,
                           const RHI::FRHISwapchainCapabilities &InCapabilities,
                           RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLSwapchain () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHISwapchainDesc &GetDesc () const noexcept override;
        [[nodiscard]] const RHI::FRHISwapchainCapabilities &GetCapabilities () const noexcept override;
        [[nodiscard]] UInt32 GetImageCount () const noexcept override;
        [[nodiscard]] TExpected<TSharedRef<RHI::IRHITexture>, RHI::FRHIError> GetImage ( UInt32 InIndex ) const noexcept override;
        [[nodiscard]] TExpected<RHI::FRHIAcquiredImage, RHI::FRHIError> AcquireNextImage () noexcept override;
        [[nodiscard]] RHI::FRHIExpected Present ( RHI::IRHIQueue &InQueue,
                                                  const RHI::FRHIAcquiredImage &InImage,
                                                  RHI::IRHISemaphore *InRenderComplete ) noexcept override;
        [[nodiscard]] RHI::FRHIExpected Resize ( const RHI::FRHIExtent3D &InExtent ) noexcept override;

    public:

        /** @brief Marks the ring out-of-date; the next acquire reports OutOfDate. */
        void MarkOutOfDate () noexcept;

    private:

        struct FSlot
        {
            FSlot ( TSharedRef<RHI::IRHITexture> InImage,
                    TSharedRef<FOpenGLSemaphore> InAcquireSemaphore,
                    TSharedRef<FOpenGLSemaphore> InPresentSemaphore ) noexcept;

            TSharedRef<RHI::IRHITexture> Image;
            TSharedRef<FOpenGLSemaphore> AcquireSemaphore;
            TSharedRef<FOpenGLSemaphore> PresentSemaphore;
            Bool bAcquired = false;
        };

        [[nodiscard]] RHI::FRHIExpected RebuildImages () noexcept;
        [[nodiscard]] RHI::FRHIExpected PresentToBridge () noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHISwapchainDesc Desc;
        TOptional<RHI::FRHINativeSurface> NativeSurface;
        RHI::FRHISwapchainCapabilities Capabilities;
        TVector<FSlot> Slots;
        UInt32 Cursor    = 0U;
        Bool bOutOfDate  = false;
        Bool bSuboptimal = false;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace RHIOpenGL

} // namespace Lumen
