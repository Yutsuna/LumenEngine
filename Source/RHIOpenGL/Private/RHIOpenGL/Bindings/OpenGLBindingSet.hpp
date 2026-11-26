/**
 * @file OpenGLBindingSet.hpp
 * @brief Declares the OpenGL binding set with atomic validated writes.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Bindings/IRHIBindingSet.hpp>

#include <RHIOpenGL/Bindings/OpenGLBindingLayout.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLBindingSet
     * @brief Validates the complete write batch before replacing its snapshot.
     */
    class FOpenGLBindingSet final : public RHI::IRHIBindingSet, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLBindingSet>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const TSharedRef<RHI::IRHIBindingLayout> &InLayout,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLBindingSet ( TSharedRef<FOpenGLDeviceContext> InContext,
                            TSharedRef<RHI::IRHIBindingLayout> InLayout,
                            RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLBindingSet () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] TSharedRef<RHI::IRHIBindingLayout> GetLayout () const noexcept override;

        [[nodiscard]] RHI::FRHIExpected Write ( TSpan<const RHI::FRHIBindingWrite> InWrites ) noexcept override;

        [[nodiscard]] Bool IsDirty () const noexcept override;

        void ClearDirty () noexcept override;

        /** @return The validated snapshot consumed by the command executor. */
        [[nodiscard]] const TVector<RHI::FRHIBindingWrite> &GetWrites () const noexcept;

        /** @return The context this set was created on. */
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        [[nodiscard]] RHI::FRHIExpected ValidateWrite ( const RHI::FRHIBindingWrite &InWrite ) const noexcept;
        [[nodiscard]] Bool OwnsBuffer ( RHI::IRHIBuffer *InBuffer ) const noexcept;
        [[nodiscard]] Bool OwnsView ( RHI::IRHITextureView *InView ) const noexcept;
        [[nodiscard]] Bool OwnsSampler ( RHI::IRHISampler *InSampler ) const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        TSharedRef<RHI::IRHIBindingLayout> Layout;
        TVector<RHI::FRHIBindingWrite> Writes;
        Bool bDirty = false;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace RHIOpenGL

} // namespace Lumen
