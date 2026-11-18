/**
 * @file NullPresentation.hpp
 * @brief Declares the deterministic Null swapchain.
 */
#pragma once

#include <Device/NullFaultPlan.hpp>
#include <HAL/PlatformMutex.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIDevice;

    namespace Tests
    {

        /** @return Capabilities accepted by every Null native surface. */
        [[nodiscard]] FRHISwapchainCapabilities MakeNullSwapchainCapabilities () noexcept;

        /**
         * @class FNullSwapchain
         * @brief CPU-owned swapchain ring with explicit acquire/present state.
         */
        class FNullSwapchain final : public IRHISwapchain
        {
        public:

            FNullSwapchain ( const FRHISwapchainDesc &InDesc,
                             const FRHISwapchainCapabilities &InCapabilities,
                             IRHIDevice &InDevice,
                             TSharedPtr<FNullFaultPlan> InFaultPlan,
                             FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;

            /** @brief Creates the initial images and binary semaphores. */
            [[nodiscard]] TExpected<void, FRHIError> Initialize () noexcept;
            const FRHISwapchainDesc &GetDesc () const noexcept override;
            const FRHISwapchainCapabilities &GetCapabilities () const noexcept override;
            UInt32 GetImageCount () const noexcept override;
            TExpected<TSharedRef<IRHITexture>, FRHIError> GetImage ( UInt32 InIndex ) const noexcept override;
            TExpected<FRHIAcquiredImage, FRHIError> AcquireNextImage () noexcept override;
            TExpected<void, FRHIError>
            Present ( IRHIQueue &InQueue, const FRHIAcquiredImage &InImage, IRHISemaphore *InRenderComplete ) noexcept override;
            TExpected<void, FRHIError> Resize ( const FRHIExtent3D &InExtent ) noexcept override;

            void MarkOutOfDate () noexcept;
            void MarkSuboptimal () noexcept;
            [[nodiscard]] UInt64 GetPresentCount () const noexcept;

        private:

            [[nodiscard]] TExpected<void, FRHIError> RebuildImages () noexcept;

        private:

            FRHIObjectBase Identity;
            FRHISwapchainDesc Desc;
            FRHISwapchainCapabilities Capabilities;
            IRHIDevice *Device;
            TSharedPtr<FNullFaultPlan> FaultPlan;
            TVector<TSharedRef<IRHITexture>> Images;
            TVector<TSharedRef<IRHISemaphore>> AcquireSemaphores;
            TVector<TSharedRef<IRHISemaphore>> PresentSemaphores;
            TVector<Bool> Acquired;
            UInt32 Cursor       = 0U;
            UInt64 PresentCount = 0ULL;
            Bool bOutOfDate     = false;
            Bool bSuboptimal    = false;
            mutable HAL::FPlatformRecursiveSharedMutex Mutex;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
