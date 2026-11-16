/**
 * @file IRHICommandList.hpp
 * @brief Declares complete backend command recording.
 */

#pragma once
#include <Bindings/IRHIBindingSet.hpp>
#include <Commands/RHICommandListState.hpp>
#include <Commands/RHICommandStream.hpp>
#include <Commands/RHICopyDescs.hpp>
#include <Commands/RHIDrawArguments.hpp>
#include <Commands/RHIRenderingDesc.hpp>
#include <Commands/RHITransition.hpp>
#include <Pipelines/IRHIPipeline.hpp>
#include <Pipelines/RHIPrimitiveTopology.hpp>
#include <Types/RHITimeline.hpp>
#include <Types/RHIViewport.hpp>
namespace Lumen
{
class IRHIBindingSet;
class IRHIQueryPool;
class IRHICommandList : public IRHIObject
{
public:

    [[nodiscard]] virtual ERHIQueueType GetQueueType () const noexcept                                             = 0;
    [[nodiscard]] virtual ERHICommandListState GetState () const noexcept                                          = 0;
    [[nodiscard]] virtual FRHITimelineValue GetCompletion () const noexcept                                        = 0;
    [[nodiscard]] virtual const FRHICommandStream &GetStream () const noexcept                                     = 0;
    virtual TExpected<void, FRHIError> Begin () noexcept                                                           = 0;
    virtual TExpected<void, FRHIError> End () noexcept                                                             = 0;
    virtual TExpected<void, FRHIError> Reset () noexcept                                                           = 0;
    virtual TExpected<void, FRHIError> MarkPending ( FRHITimelineValue InCompletion ) noexcept                     = 0;
    virtual TExpected<void, FRHIError> MarkCompleted ( FRHITimelineValue InCompleted ) noexcept                    = 0;
    virtual TExpected<void, FRHIError> Transition ( TSpan<const FRHITransition> InTransitions ) noexcept           = 0;
    virtual TExpected<void, FRHIError> CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                    TSharedRef<IRHIBuffer> InDestination,
                                                    const FRHIBufferCopy &InCopy ) noexcept                        = 0;
    virtual TExpected<void, FRHIError> CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                     TSharedRef<IRHITexture> InDestination,
                                                     const FRHITextureCopy &InCopy ) noexcept                      = 0;
    virtual TExpected<void, FRHIError> CopyBufferToTexture ( TSharedRef<IRHIBuffer> InSource,
                                                             TSharedRef<IRHITexture> InDestination,
                                                             const FRHIBufferTextureCopy &InCopy ) noexcept        = 0;
    virtual TExpected<void, FRHIError> CopyTextureToBuffer ( TSharedRef<IRHITexture> InSource,
                                                             TSharedRef<IRHIBuffer> InDestination,
                                                             const FRHIBufferTextureCopy &InCopy ) noexcept        = 0;
    virtual TExpected<void, FRHIError> BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept                 = 0;
    virtual TExpected<void, FRHIError> EndRendering () noexcept                                                    = 0;
    virtual TExpected<void, FRHIError> SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept                = 0;
    virtual TExpected<void, FRHIError> SetBindingSet ( UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept = 0;
    virtual TExpected<void, FRHIError>
    SetVertexBuffer ( UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept = 0;
    virtual TExpected<void, FRHIError>
    SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset, ERHIIndexFormat InFormat ) noexcept  = 0;
    virtual TExpected<void, FRHIError> Draw ( const FRHIDrawArguments &InArguments ) noexcept               = 0;
    virtual TExpected<void, FRHIError> DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept = 0;
    virtual TExpected<void, FRHIError> DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                      const FRHIIndirectArguments &InDesc ) noexcept        = 0;
    virtual TExpected<void, FRHIError> Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept       = 0;
    virtual TExpected<void, FRHIError> DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                          const FRHIIndirectArguments &InDesc ) noexcept    = 0;
    virtual TExpected<void, FRHIError> SetViewport ( const FRHIViewport &InViewport ) noexcept              = 0;
    virtual TExpected<void, FRHIError> SetScissor ( const FRHIRect2D &InScissor ) noexcept                  = 0;
    virtual TExpected<void, FRHIError> SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept     = 0;
    virtual TExpected<void, FRHIError> SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept        = 0;
    virtual TExpected<void, FRHIError> SetStencilReference ( UInt32 InReference ) noexcept                  = 0;
    virtual TExpected<void, FRHIError> BeginQuery ( IRHIQueryPool &InPool, UInt32 InIndex ) noexcept        = 0;
    virtual TExpected<void, FRHIError> EndQuery ( IRHIQueryPool &InPool, UInt32 InIndex ) noexcept          = 0;
    virtual TExpected<void, FRHIError> BeginDebugRegion ( FStringView InName ) noexcept                     = 0;
    virtual TExpected<void, FRHIError> EndDebugRegion () noexcept                                           = 0;
};
} // namespace Lumen
