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
     * @brief A fixed-size array of GL query objects with executor-published results.
     * @details Occlusion uses SamplesPassed; timestamp uses an absolute
     *          glQueryCounter(GL_TIMESTAMP) value. Both produce one UInt64 per
     *          query. Recording only mutates CPU state: all native calls and
     *          result publication happen on the GL Executor. Pipeline statistics
     *          are rejected at creation because GL has no portable equivalent.
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
        [[nodiscard]] GLenum GetNativeTarget () const noexcept;
        [[nodiscard]] UInt32 GetGeneration ( UInt32 InIndex ) const noexcept;
        /** @brief Validates and marks a query active; no native GL call is made. */
        [[nodiscard]] RHI::FRHIExpected Begin ( UInt32 InIndex ) noexcept;
        /** @brief Marks a query pending publication; no native GL call is made. */
        [[nodiscard]] RHI::FRHIExpected End ( UInt32 InIndex ) noexcept;
        /** @brief Publishes an executor result, gated by the submission completion. */
        [[nodiscard]] RHI::FRHIExpected
        PublishResult ( UInt32 InIndex, UInt32 InGeneration, UInt64 InValue, RHI::FRHITimelineValue InCompletion ) noexcept;
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIQueryPoolDesc Desc;
        TVector<GLuint> Queries;
        TVector<Bool> Active;
        TVector<Bool> Pending;
        TVector<Bool> Available;
        TVector<UInt32> Generations;
        TVector<UInt64> Results;
        TVector<RHI::FRHITimelineValue> Availability;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace RHIOpenGL

} // namespace Lumen
