/**
 * @file RHIValidationCommandList.hpp
 * @brief Declares full command recording decoration and state validation.
 */
#pragma once

#include "RHIValidationState.hpp"
#include <Commands/IRHICommandList.hpp>

namespace Lumen
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
    [[nodiscard]] TExpected<void, FRHIError> Begin () noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> End () noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Reset () noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> MarkPending ( FRHITimelineValue InCompletion ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> MarkCompleted ( FRHITimelineValue InCompleted ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Transition ( TSpan<const FRHITransition> InTransitions ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                          TSharedRef<IRHIBuffer> InDestination,
                                                          const FRHIBufferCopy &InCopy ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                           TSharedRef<IRHITexture> InDestination,
                                                           const FRHITextureCopy &InCopy ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> CopyBufferToTexture ( TSharedRef<IRHIBuffer> InSource,
                                                                   TSharedRef<IRHITexture> InDestination,
                                                                   const FRHIBufferTextureCopy &InCopy ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> CopyTextureToBuffer ( TSharedRef<IRHITexture> InSource,
                                                                   TSharedRef<IRHIBuffer> InDestination,
                                                                   const FRHIBufferTextureCopy &InCopy ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> EndRendering () noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetBindingSet ( UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError>
    SetVertexBuffer ( UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError>
    SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset, ERHIIndexFormat InFormat ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Draw ( const FRHIDrawArguments &InArguments ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                            const FRHIIndirectArguments &InDesc ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                                const FRHIIndirectArguments &InDesc ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetViewport ( const FRHIViewport &InViewport ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetScissor ( const FRHIRect2D &InScissor ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> SetStencilReference ( UInt32 InReference ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> BeginQuery ( TSharedRef<IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> EndQuery ( TSharedRef<IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> BeginDebugRegion ( FStringView InName ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> EndDebugRegion () noexcept override;
    /** @brief Clears decorator-local bindings after the underlying allocator recycled this list. */
    void OnAllocatorReset () noexcept;

private:

    /** @return True when command-state validation is enabled by the device descriptor. */
    [[nodiscard]] Bool IsCommandValidationEnabled () const noexcept;
    [[nodiscard]] TExpected<void, FRHIError> CheckRecording ( Bool bAllowRendering ) const noexcept;
    [[nodiscard]] TExpected<void, FRHIError> CheckRendering () const noexcept;
    [[nodiscard]] TExpected<void, FRHIError> ValidateBuffer ( const IRHIBuffer &InBuffer ) const noexcept;
    [[nodiscard]] TExpected<void, FRHIError> ValidateTexture ( const IRHITexture &InTexture ) const noexcept;
    [[nodiscard]] TExpected<void, FRHIError> CheckDraw ( ERHIPipelineType InType, Bool bRendering ) const noexcept;

private:

    TSharedRef<IRHICommandList> Underlying;
    TSharedPtr<FRHIValidationCommandAllocator> Allocator;
    TSharedPtr<FRHIValidationState> State;
    TOptional<ERHIPipelineType> BoundPipeline;
    IRHIQueryPool *ActiveQueryPool = nullptr;
    UInt32 ActiveQueryIndex        = 0U;
    UInt32 DebugDepth              = 0U;
};

} // namespace Lumen
