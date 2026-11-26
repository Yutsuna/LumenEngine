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

#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Function.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

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
     * @brief Builds valid defrag plans; native moves are a later milestone.
     */
    class FOpenGLDefragmenter final : public RHI::IRHIDefragmenter, public FNonCopyable, public FNonMovable
    {
    public:

        explicit FOpenGLDefragmenter ( RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLDefragmenter () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] TExpected<RHI::FRHIDefragPlan, RHI::FRHIError>
        BuildPlan ( TSpan<const RHI::FRHIAllocation> InAllocations ) noexcept override;
        [[nodiscard]] RHI::FRHIExpected Execute ( const RHI::FRHIDefragPlan &InPlan ) noexcept override;

    private:

        RHI::FRHIObjectBase Identity;
    };

} // namespace RHIOpenGL

} // namespace Lumen
