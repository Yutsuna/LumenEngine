/**
 * @file IRHISurfaceBridge.hpp
 * @brief Declares backend-neutral native surface and context operations.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Memory/Span.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <RHIExport.hpp>
#include <String/StringView.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @struct FRHINativeSurface
     * @brief Opaque native handles interpreted only by a backend or bridge implementation.
     */
    struct FRHINativeSurface
    {
        UPtr BackendHandle = 0U;
        UPtr SurfaceHandle = 0U;
        UPtr ContextHandle = 0U;
    };

    /**
     * @class IRHISurfaceBridge
     * @brief Creates native surfaces and supplies context, presentation, and procedure operations.
     */
    class IRHISurfaceBridge
    {
    public:

        virtual ~IRHISurfaceBridge () noexcept = default;

        [[nodiscard]] virtual TExpected<FRHINativeSurface, FRHIError> CreateNativeSurface ( UPtr InBackendHandle ) noexcept = 0;
        virtual void DestroyNativeSurface ( const FRHINativeSurface &InSurface ) noexcept                                   = 0;
        [[nodiscard]] virtual FRHIExpected MakeCurrent ( const FRHINativeSurface &InSurface ) noexcept                      = 0;
        [[nodiscard]] virtual FRHIExpected Present ( const FRHINativeSurface &InSurface, Int32 InSwapInterval ) noexcept    = 0;
        [[nodiscard]] virtual TExpected<FRHIExtent3D, FRHIError> GetExtent () const noexcept                                = 0;
        [[nodiscard]] virtual TOptional<UPtr> ResolveProcedure ( FStringView InName ) noexcept                              = 0;
    };
} // namespace RHI

} // namespace Lumen
