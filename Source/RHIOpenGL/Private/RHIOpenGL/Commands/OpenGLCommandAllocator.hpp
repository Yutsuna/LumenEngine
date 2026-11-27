/**
 * @file OpenGLCommandAllocator.hpp
 * @brief Declares the OpenGL command allocator and list recycling state.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Commands/RHICommandAllocatorBase.hpp>

#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    class FOpenGLCommandList;

    /**
     * @class FOpenGLCommandAllocator
     * @brief Tracks command lists for one logical queue and recycles completed ones.
     */
    class FOpenGLCommandAllocator final : public RHI::FRHICommandAllocatorBase, public FNonCopyable, public FNonMovable
    {
    public:

        FOpenGLCommandAllocator ( RHI::ERHIQueueType InQueueType, RHI::FRHIObjectId InId ) noexcept;
        ~FOpenGLCommandAllocator () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] RHI::FRHIExpected Reset ( RHI::FRHITimelineValue InCompleted ) noexcept override;

    public:

        void TrackList ( FOpenGLCommandList &InList ) noexcept;
        void UntrackList ( FOpenGLCommandList &InList ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TVector<FOpenGLCommandList *> Lists;
    };

} // namespace RHIOpenGL

} // namespace Lumen
