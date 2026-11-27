/**
 * @file OpenGLMemoryObjects.hpp
 * @brief Declares OpenGL buffer recycling, transient placement and defrag planning.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>

#include <RHI/Memory/IRHIBufferPool.hpp>
#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Loader/GLSpec.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Function.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @brief Decides whether a defragmentation move must be staged through a scratch buffer.
     * @details Overlapping source/destination ranges are hazardous only within the same native
     *          GL buffer. Distinct heaps own distinct buffers and can be copied directly even
     *          when their heap-relative offsets happen to coincide.
     * @return True when the copy needs a scratch buffer.
     */
    [[nodiscard]] Bool NeedsScratchCopy ( GLuint InSourceBuffer,
                                          GLuint InDestinationBuffer,
                                          UInt64 InSourceOffset,
                                          UInt64 InSourceSize,
                                          UInt64 InDestinationOffset,
                                          UInt64 InDestinationSize ) noexcept;

    /**
     * @class FOpenGLBufferPool
     * @brief Recycles compatible buffers only after their completion timeline.
     */
    class FOpenGLBufferPool final : public RHI::IRHIBufferPool, public FNonCopyable, public FNonMovable
    {
    public:

        FOpenGLBufferPool (
            TFunction<TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>( const RHI::FRHIBufferDesc & )> InCreator,
            RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLBufferPool () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>
        Acquire ( const RHI::FRHIBufferDesc &InDesc ) noexcept override;
        [[nodiscard]] RHI::FRHIExpected Release ( TSharedRef<RHI::IRHIBuffer> InBuffer,
                                                  RHI::FRHITimelineValue InCompletion ) noexcept override;
        void AdvanceCompleted ( RHI::FRHITimelineValue InCompleted ) noexcept override;
        [[nodiscard]] USize GetPendingCount () const noexcept override;

    private:

        struct FPendingBuffer
        {
            TSharedRef<RHI::IRHIBuffer> Buffer;
            RHI::FRHITimelineValue Completion;
        };

    private:

        RHI::FRHIObjectBase Identity;
        TFunction<TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>( const RHI::FRHIBufferDesc & )> Creator;
        mutable HAL::FPlatformMutex Mutex;
        TVector<TSharedRef<RHI::IRHIBuffer>> Available;
        TVector<FPendingBuffer> Pending;
        TVector<TSharedRef<RHI::IRHIBuffer>> Owned;
    };

    /**
     * @class FOpenGLTransientResourceAllocator
     * @brief Interval-colors transient requests into non-aliasing offsets.
     */
    class FOpenGLTransientResourceAllocator final : public RHI::IRHITransientResourceAllocator,
                                                    public FNonCopyable,
                                                    public FNonMovable
    {
    public:

        explicit FOpenGLTransientResourceAllocator ( RHI::FRHIObjectId InId, UInt64 InMinAlignment ) noexcept;
        ~FOpenGLTransientResourceAllocator () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        void BeginBatch () noexcept override;
        [[nodiscard]] TExpected<TVector<RHI::FRHITransientAllocation>, RHI::FRHIError>
        Allocate ( const RHI::FRHITransientBatch &InBatch ) noexcept override;
        void EndBatch () noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        UInt64 MinAlignment = 1ULL;
        Bool bInBatch       = false;
        UInt64 NextId       = 1ULL;
        HAL::FPlatformMutex Mutex;
    };

    /**
     * @class FOpenGLDefragmenter
     * @brief Builds validated plans and executes native heap moves on the GL executor.
     */
    class FOpenGLDefragmenter final : public RHI::IRHIDefragmenter, public FNonCopyable, public FNonMovable
    {
    public:

        FOpenGLDefragmenter ( TSharedRef<FOpenGLDeviceContext> InContext, RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLDefragmenter () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] TExpected<RHI::FRHIDefragPlan, RHI::FRHIError>
        BuildPlan ( TSpan<const RHI::FRHIAllocation> InAllocations ) noexcept override;
        /**
         * @brief Executes a validated defragmentation plan as native heap copies on the GL executor.
         * @details The caller must populate @c InPlan.ObservedCompletion, typically after calling
         *          WaitIdle() on every queue that produced the source allocations, and every move's
         *          completion must be reached or the call fails with InvalidState. The call is
         *          synchronous with respect to the executor: the copies are visible on return, but
         *          no new submission timeline value is created. Allocation observers are notified
         *          only after all copies succeed, and never on failure.
         * @return An empty expected on success, otherwise a structured RHI error.
         */
        [[nodiscard]] RHI::FRHIExpected Execute ( const RHI::FRHIDefragPlan &InPlan ) noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
    };

} // namespace RHIOpenGL

} // namespace Lumen
