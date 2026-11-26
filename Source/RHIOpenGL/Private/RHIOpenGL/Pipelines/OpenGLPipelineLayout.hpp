/**
 * @file OpenGLPipelineLayout.hpp
 * @brief Declares the immutable OpenGL pipeline layout.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>

#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @struct FOpenGLUnitRange
     * @brief Half-open range of absolute OpenGL binding units reserved for one resource class.
     */
    struct FOpenGLUnitRange
    {
        UInt32 Base  = 0U;
        UInt32 Count = 0U;
    };

    /**
     * @struct FOpenGLBindingSetUnits
     * @brief Deterministic flat unit assignment for one descriptor set.
     */
    struct FOpenGLBindingSetUnits
    {
        FOpenGLUnitRange UniformBuffers;
        FOpenGLUnitRange StorageBuffers;
        FOpenGLUnitRange Textures;
        FOpenGLUnitRange Images;
    };

    /**
     * @class FOpenGLPipelineLayout
     * @brief Validated binding layout set shared by OpenGL pipelines and binding sets.
     */
    class FOpenGLPipelineLayout final : public RHI::IRHIPipelineLayout, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLPipelineLayout>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                 const RHI::FRHIPipelineLayoutDesc &InDesc,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLPipelineLayout ( TSharedRef<FOpenGLDeviceContext> InContext,
                                const RHI::FRHIPipelineLayoutDesc &InDesc,
                                RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLPipelineLayout () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] const RHI::FRHIPipelineLayoutDesc &GetDesc () const noexcept override;

        /** @return Stable fold of every backend binding layout descriptor hash. */
        [[nodiscard]] UInt64 GetLayoutHash () const noexcept;

        /** @return The context this layout was created for. */
        [[nodiscard]] const FOpenGLDeviceContext &GetDeviceContext () const noexcept;

        /** @return Immutable flat unit assignment for a descriptor set, or empty when out of range. */
        [[nodiscard]] TOptional<FOpenGLBindingSetUnits> GetBindingSetUnits ( UInt32 InSetIndex ) const noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHIPipelineLayoutDesc Desc;
        UInt64 LayoutHash = 0ULL;
        TVector<FOpenGLBindingSetUnits> BindingSetUnits;
    };

} // namespace RHIOpenGL

} // namespace Lumen
