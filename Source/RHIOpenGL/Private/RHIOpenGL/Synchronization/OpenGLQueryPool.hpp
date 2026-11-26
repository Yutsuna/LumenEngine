/**
 * @file OpenGLQueryPool.hpp
 * @brief Declares OpenGL query pools with CPU-visible completed results.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Queries/IRHIQueryPool.hpp>
#include <RHI/Queries/RHIQueryPoolDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHITimeline.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLQueryPool
     * @brief A fixed-size array of GL query objects read back on the GL executor.
     * @details Occlusion uses SamplesPassed and timestamp uses TimeElapsed; both
     *          produce one UInt64 per query. Pipeline statistics are rejected at
     *          creation because GL has no portable equivalent.
     */
    class FOpenGLQueryPool final : public RHI::IRHIQueryPool, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLQueryPool>, RHI::FRHIError> Create (
            TSharedRef<FOpenGLDeviceContext> InContext, const RHI::FRHIQueryPoolDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLQueryPool ( TSharedRef<FOpenGLDeviceContext> InContext,
                           const RHI::FRHIQueryPoolDesc &InDesc,
                           RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLQueryPool () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIQueryPoolDesc &GetDesc () const noexcept override;
        void Reset () noexcept override;
        [[nodiscard]] RHI::FRHIExpected ReadResults ( UInt32 InFirstQuery,
                                                      UInt32 InQueryCount,
                                                      TSpan<UInt64> InDestination,
                                                      RHI::FRHITimelineValue InCompleted ) const noexcept override;

    public:

        [[nodiscard]] GLuint GetQuery ( UInt32 InIndex ) const noexcept;
        [[nodiscard]] RHI::FRHIExpected Begin ( UInt32 InIndex ) noexcept;
        [[nodiscard]] RHI::FRHIExpected End ( UInt32 InIndex ) noexcept;
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIQueryPoolDesc Desc;
        TVector<GLuint> Queries;
        TVector<Bool> Active;
        TVector<Bool> Available;
        TVector<UInt64> Results;
        TVector<RHI::FRHITimelineValue> Availability;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace RHIOpenGL

} // namespace Lumen
