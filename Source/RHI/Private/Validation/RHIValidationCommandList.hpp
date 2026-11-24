/**
 * @file RHIValidationCommandList.hpp
 * @brief Declares full command recording decoration and state validation.
 */

#pragma once

#include "RHIValidationState.hpp"
#include <RHI/Commands/IRHICommandList.hpp>

namespace Lumen
{

namespace RHI
{

    class FRHIValidationCommandAllocator;

    /**
     * @class FRHIValidationCommandList
     * @brief Validates command order, resource ownership, and recording scopes.
     */
    class FRHIValidationCommandList final : public IRHICommandList, public IRHIValidationObject
    {
    public:

        FRHIValidationCommandList ( TSharedRef<IRHICommandList> InUnderlying,
                                    TSharedPtr<FRHIValidationCommandAllocator> InAllocator,
                                    TSharedPtr<FRHIValidationState> InState ) noexcept;

        ~FRHIValidationCommandList () noexcept override;

    public:

        [[nodiscard]] FRHIObjectId GetId () const noexcept override;

        [[nodiscard]] FString GetDebugName () const noexcept override;

        void SetDebugName ( FStringView InName ) noexcept override;

        [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;

        [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;

        [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;

        [[nodiscard]] ERHIQueueType GetQueueType () const noexcept override;

        [[nodiscard]] ERHICommandListState GetState () const noexcept override;

        [[nodiscard]] FRHITimelineValue GetCompletion () const noexcept override;

        [[nodiscard]] const FRHICommandStream &GetStream () const noexcept override;

        [[nodiscard]] FRHIExpected Begin () noexcept override;

        [[nodiscard]] FRHIExpected End () noexcept override;

        [[nodiscard]] FRHIExpected Reset () noexcept override;

        [[nodiscard]] FRHIExpected MarkPending ( FRHITimelineValue InCompletion ) noexcept override;

        [[nodiscard]] FRHIExpected MarkCompleted ( FRHITimelineValue InCompleted ) noexcept override;

        [[nodiscard]] FRHIExpected Transition ( TSpan<const FRHITransition> InTransitions ) noexcept override;

        [[nodiscard]] FRHIExpected CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                TSharedRef<IRHIBuffer> InDestination,
                                                const FRHIBufferCopy &InCopy ) noexcept override;

        [[nodiscard]] FRHIExpected CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                 TSharedRef<IRHITexture> InDestination,
                                                 const FRHITextureCopy &InCopy ) noexcept override;

        [[nodiscard]] FRHIExpected CopyBufferToTexture ( TSharedRef<IRHIBuffer> InSource,
                                                         TSharedRef<IRHITexture> InDestination,
                                                         const FRHIBufferTextureCopy &InCopy ) noexcept override;

        [[nodiscard]] FRHIExpected CopyTextureToBuffer ( TSharedRef<IRHITexture> InSource,
                                                         TSharedRef<IRHIBuffer> InDestination,
                                                         const FRHIBufferTextureCopy &InCopy ) noexcept override;

        [[nodiscard]] FRHIExpected BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept override;

        [[nodiscard]] FRHIExpected EndRendering () noexcept override;

        [[nodiscard]] FRHIExpected SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept override;

        [[nodiscard]] FRHIExpected SetBindingSet ( UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept override;

        [[nodiscard]] FRHIExpected
        SetInlineConstants ( ERHIShaderStage InStages, UInt32 InOffset, TSpan<const Byte> InValues ) noexcept override;

        [[nodiscard]] FRHIExpected
        SetVertexBuffer ( UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept override;

        [[nodiscard]] FRHIExpected
        SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset, ERHIIndexFormat InFormat ) noexcept override;

        [[nodiscard]] FRHIExpected Draw ( const FRHIDrawArguments &InArguments ) noexcept override;

        [[nodiscard]] FRHIExpected DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept override;

        [[nodiscard]] FRHIExpected DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                  const FRHIIndirectArguments &InDesc ) noexcept override;

        [[nodiscard]] FRHIExpected Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept override;

        [[nodiscard]] FRHIExpected DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                      const FRHIIndirectArguments &InDesc ) noexcept override;

        [[nodiscard]] FRHIExpected SetViewport ( const FRHIViewport &InViewport ) noexcept override;

        [[nodiscard]] FRHIExpected SetScissor ( const FRHIRect2D &InScissor ) noexcept override;

        [[nodiscard]] FRHIExpected SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept override;

        [[nodiscard]] FRHIExpected SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept override;

        [[nodiscard]] FRHIExpected SetStencilReference ( UInt32 InReference ) noexcept override;

        [[nodiscard]] FRHIExpected BeginQuery ( TSharedRef<IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;

        [[nodiscard]] FRHIExpected EndQuery ( TSharedRef<IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;

        [[nodiscard]] FRHIExpected BeginDebugRegion ( FStringView InName ) noexcept override;

        [[nodiscard]] FRHIExpected EndDebugRegion () noexcept override;

        /** @brief Clears decorator-local bindings after the underlying allocator recycled this list. */
        void OnAllocatorReset () noexcept;

    private:

        /** @return True when command-state validation is enabled by the device descriptor. */
        [[nodiscard]] Bool IsCommandValidationEnabled () const noexcept;

        [[nodiscard]] FRHIExpected CheckRecording ( Bool bAllowRendering ) const noexcept;

        [[nodiscard]] FRHIExpected CheckRendering () const noexcept;

        [[nodiscard]] FRHIExpected ValidateBuffer ( const IRHIBuffer &InBuffer ) const noexcept;

        [[nodiscard]] FRHIExpected ValidateTexture ( const IRHITexture &InTexture ) const noexcept;

        [[nodiscard]] FRHIExpected CheckDraw ( ERHIPipelineType InType, Bool bRendering ) const noexcept;

    private:

        TSharedRef<IRHICommandList> Underlying;
        TSharedPtr<FRHIValidationCommandAllocator> Allocator;
        TSharedPtr<FRHIValidationState> State;
        TOptional<ERHIPipelineType> BoundPipeline;

        IRHIQueryPool *ActiveQueryPool = nullptr;
        UInt32 ActiveQueryIndex        = 0U;
        UInt32 DebugDepth              = 0U;
    };

} // namespace RHI

} // namespace Lumen
