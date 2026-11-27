/**
 * @file NullSurfaceBridge.hpp
 * @brief Declares a deterministic test implementation of the native surface bridge.
 */
#pragma once

#include <RHI/Presentation/IRHISurfaceBridge.hpp>

namespace Lumen
{

namespace RHI
{
    namespace Tests
    {
        /**
         * @class FNullSurfaceBridge
         * @brief Simulates native surface creation, context operations, and resolution.
         */
        class FNullSurfaceBridge final : public IRHISurfaceBridge
        {
        public:

            explicit FNullSurfaceBridge ( FRHIExtent3D InExtent = { 1U, 1U, 1U } ) noexcept;
            TExpected<FRHINativeSurface, FRHIError> CreateNativeSurface ( UPtr InBackendHandle ) noexcept override;
            void DestroyNativeSurface ( const FRHINativeSurface &InSurface ) noexcept override;
            FRHIExpected MakeCurrent ( const FRHINativeSurface &InSurface ) noexcept override;
            FRHIExpected Present ( const FRHINativeSurface &InSurface, Int32 InSwapInterval ) noexcept override;
            TExpected<FRHIExtent3D, FRHIError> GetExtent () const noexcept override;
            TOptional<UPtr> ResolveProcedure ( FStringView InName ) noexcept override;
            [[nodiscard]] UInt32 GetCreateCount () const noexcept;
            [[nodiscard]] UInt32 GetDestroyCount () const noexcept;

        private:

            FRHIExtent3D Extent;
            UInt32 CreateCount  = 0U;
            UInt32 DestroyCount = 0U;
        };
    } // namespace Tests
} // namespace RHI

} // namespace Lumen
