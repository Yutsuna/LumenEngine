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
/**
 * @class IRHICommandList
 * @brief A re-usable recorder of graphics, compute and copy commands.
 * @details A command list is confined to one recording thread; begin/end,
 *          rendering scope and state changes are externally ordered by that
 *          thread and must not be recorded concurrently. It moves through
 *          Initial, Recording, Rendering, Executable and Pending states; any
 *          illegal transition leaves the object unchanged and returns
 *          InvalidState. GetStream exposes the recorded commands, and the list
 *          retains every referenced resource until the owning allocator recycles
 *          it after GPU completion. Indirect draws, queries and bindless access
 *          are gated by the corresponding device capabilities and return
 *          Unsupported when the device did not advertise them. No operation
 *          throws; every failure is returned as an FRHIError value.
 */
class IRHICommandList : public IRHIObject
{
public:

    /**
     * @brief Reports the queue family this list was created for.
     * @return The queue type fixed at creation.
     */
    [[nodiscard]] virtual ERHIQueueType GetQueueType () const noexcept = 0;

    /**
     * @brief Reports the current state of the recording state machine.
     * @return The state, which changes only through the mutators below.
     */
    [[nodiscard]] virtual ERHICommandListState GetState () const noexcept = 0;

    /**
     * @brief Reports the timeline value at which pending work completes.
     * @return The pending completion value, or an invalid value when not pending.
     */
    [[nodiscard]] virtual FRHITimelineValue GetCompletion () const noexcept = 0;

    /**
     * @brief Exposes the recorded command stream.
     * @return A const reference owned by the list; valid until Reset or destruction.
     */
    [[nodiscard]] virtual const FRHICommandStream &GetStream () const noexcept = 0;

    /**
     * @brief Opens a recording scope from Initial or Executable state.
     * @return Success, or InvalidState if the current state forbids Begin.
     */
    virtual TExpected<void, FRHIError> Begin () noexcept = 0;

    /**
     * @brief Closes recording and makes the list executable.
     * @return Success, or InvalidState if a query or rendering scope is still open.
     */
    virtual TExpected<void, FRHIError> End () noexcept = 0;

    /**
     * @brief Clears recorded commands and returns the list to Initial state.
     * @return Success, or InvalidState if the list is Pending or has an open query.
     */
    virtual TExpected<void, FRHIError> Reset () noexcept = 0;

    /**
     * @brief Marks an executable list as submitted and pending completion.
     * @param InCompletion Valid, nonzero timeline value for the submitted work.
     * @return Success, or InvalidArgument for an invalid value and InvalidState if
     *         the list is not executable.
     */
    virtual TExpected<void, FRHIError> MarkPending ( FRHITimelineValue InCompletion ) noexcept = 0;

    /**
     * @brief Marks a pending list as completed once the timeline is reached.
     * @param InCompleted Timeline value observed as reached by the caller.
     * @return Success, or InvalidState if the value was not reached or the list is
     *         not pending.
     */
    virtual TExpected<void, FRHIError> MarkCompleted ( FRHITimelineValue InCompleted ) noexcept = 0;

    /**
     * @brief Records resource state transitions.
     * @param InTransitions Transitions to validate and record in order.
     * @return Success, or InvalidState outside a recording scope, InvalidArgument
     *         for an invalid transition, or CorruptedData for a malformed variant.
     */
    virtual TExpected<void, FRHIError> Transition ( TSpan<const FRHITransition> InTransitions ) noexcept = 0;

    /**
     * @brief Records a buffer-to-buffer copy.
     * @param InSource Buffer with CopySource usage.
     * @param InDestination Buffer with CopyDestination usage.
     * @param InCopy Validated copy range and offsets.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid range or missing usage.
     */
    virtual TExpected<void, FRHIError> CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                    TSharedRef<IRHIBuffer> InDestination,
                                                    const FRHIBufferCopy &InCopy ) noexcept = 0;

    /**
     * @brief Records a texture-to-texture copy.
     * @param InSource Texture with CopySource usage.
     * @param InDestination Texture with CopyDestination usage.
     * @param InCopy Validated subresource range, format and extent.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid range or missing usage.
     */
    virtual TExpected<void, FRHIError> CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                     TSharedRef<IRHITexture> InDestination,
                                                     const FRHITextureCopy &InCopy ) noexcept = 0;

    /**
     * @brief Records a buffer-to-texture copy.
     * @param InSource Buffer with CopySource usage.
     * @param InDestination Texture with CopyDestination usage.
     * @param InCopy Validated layout, footprint and subresource range.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid layout or missing usage.
     */
    virtual TExpected<void, FRHIError> CopyBufferToTexture ( TSharedRef<IRHIBuffer> InSource,
                                                             TSharedRef<IRHITexture> InDestination,
                                                             const FRHIBufferTextureCopy &InCopy ) noexcept = 0;

    /**
     * @brief Records a texture-to-buffer copy.
     * @param InSource Texture with CopySource usage.
     * @param InDestination Buffer with CopyDestination usage.
     * @param InCopy Validated layout, footprint and subresource range.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid layout or missing usage.
     */
    virtual TExpected<void, FRHIError> CopyTextureToBuffer ( TSharedRef<IRHITexture> InSource,
                                                             TSharedRef<IRHIBuffer> InDestination,
                                                             const FRHIBufferTextureCopy &InCopy ) noexcept = 0;

    /**
     * @brief Opens a dynamic rendering scope.
     * @param InDesc Validated attachment formats, extents and load/store operations.
     * @return Success, or InvalidState when nesting is illegal and InvalidArgument
     *         for an invalid attachment description.
     */
    virtual TExpected<void, FRHIError> BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept = 0;

    /**
     * @brief Closes the active rendering scope.
     * @return Success, or InvalidState when no rendering scope is open.
     */
    virtual TExpected<void, FRHIError> EndRendering () noexcept = 0;

    /**
     * @brief Binds a pipeline compatible with the current layout and scope.
     * @param InPipeline Pipeline whose layout must match the bound binding sets.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         incompatible pipeline.
     */
    virtual TExpected<void, FRHIError> SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept = 0;

    /**
     * @brief Binds a set to a pipeline-layout slot.
     * @param InIndex Slot index within the active pipeline layout.
     * @param InSet Set whose layout must match that slot.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         out-of-range index or mismatched layout.
     */
    virtual TExpected<void, FRHIError> SetBindingSet ( UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept = 0;

    /**
     * @brief Binds a vertex buffer to an input slot.
     * @param InSlot Vertex input slot index.
     * @param InBuffer Buffer with VertexBuffer usage.
     * @param InOffset Byte offset into the buffer.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid slot, usage or offset.
     */
    virtual TExpected<void, FRHIError>
    SetVertexBuffer ( UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept = 0;

    /**
     * @brief Binds an index buffer and its element format.
     * @param InBuffer Buffer with IndexBuffer usage.
     * @param InOffset Byte offset into the buffer.
     * @param InFormat Element format of the index data.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid usage, offset or format.
     */
    virtual TExpected<void, FRHIError>
    SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset, ERHIIndexFormat InFormat ) noexcept = 0;

    /**
     * @brief Records a non-indexed draw inside a rendering scope.
     * @param InArguments Vertex count, instance count and first indices.
     * @return Success, or InvalidState outside rendering and InvalidArgument for
     *         malformed arguments.
     */
    virtual TExpected<void, FRHIError> Draw ( const FRHIDrawArguments &InArguments ) noexcept = 0;

    /**
     * @brief Records an indexed draw inside a rendering scope.
     * @param InArguments Index count, instance count and first indices.
     * @return Success, or InvalidState outside rendering and InvalidArgument for
     *         malformed arguments.
     */
    virtual TExpected<void, FRHIError> DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept = 0;

    /**
     * @brief Records an indirect draw; gated by the IndirectDraw capability.
     * @param InArguments Buffer containing the draw arguments.
     * @param InDesc Indirect argument layout, count and offsets.
     * @return Success, or InvalidState outside rendering, InvalidArgument for a bad
     *         descriptor, or Unsupported when the capability is absent.
     */
    virtual TExpected<void, FRHIError> DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                      const FRHIIndirectArguments &InDesc ) noexcept = 0;

    /**
     * @brief Records a compute dispatch.
     * @param InArguments Workgroup counts for the three dimensions.
     * @return Success, or InvalidState outside a recording scope and InvalidArgument
     *         for malformed arguments.
     */
    virtual TExpected<void, FRHIError> Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept = 0;

    /**
     * @brief Records an indirect compute dispatch; gated by the IndirectDraw capability.
     * @param InArguments Buffer containing the dispatch arguments.
     * @param InDesc Indirect argument layout, count and offsets.
     * @return Success, or InvalidState outside recording, InvalidArgument for a bad
     *         descriptor, or Unsupported when the capability is absent.
     */
    virtual TExpected<void, FRHIError> DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                          const FRHIIndirectArguments &InDesc ) noexcept = 0;

    /**
     * @brief Sets the viewport transform as dynamic pipeline state.
     * @param InViewport Viewport origin, extent and depth range.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid viewport.
     */
    virtual TExpected<void, FRHIError> SetViewport ( const FRHIViewport &InViewport ) noexcept = 0;

    /**
     * @brief Sets the scissor rectangle as dynamic pipeline state.
     * @param InScissor Scissor rectangle in framebuffer coordinates.
     * @return Success, or InvalidState outside recording and InvalidArgument for an
     *         invalid rectangle.
     */
    virtual TExpected<void, FRHIError> SetScissor ( const FRHIRect2D &InScissor ) noexcept = 0;

    /**
     * @brief Sets the four blend factor constants as dynamic state.
     * @param InConstants Array of four blend factor components.
     * @return Success, or InvalidState when dynamic blend constants are not allowed.
     */
    virtual TExpected<void, FRHIError> SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept = 0;

    /**
     * @brief Sets the depth bias as dynamic pipeline state.
     * @param InConstant Constant depth bias factor.
     * @param InSlope Slope-scaled depth bias factor.
     * @return Success, or InvalidState when dynamic depth bias is not allowed.
     */
    virtual TExpected<void, FRHIError> SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept = 0;

    /**
     * @brief Sets the stencil reference value as dynamic pipeline state.
     * @param InReference Stencil reference value.
     * @return Success, or InvalidState when dynamic stencil state is not allowed.
     */
    virtual TExpected<void, FRHIError> SetStencilReference ( UInt32 InReference ) noexcept = 0;

    /**
     * @brief Begins a query inside a pool; gated by the pool's query capability.
     * @param InPool Pool that owns the query index.
     * @param InIndex Index within the pool.
     * @return Success, or InvalidState if a query is already active, InvalidArgument
     *         for an invalid index, or Unsupported when the type is not negotiated.
     */
    virtual TExpected<void, FRHIError> BeginQuery ( IRHIQueryPool &InPool, UInt32 InIndex ) noexcept = 0;

    /**
     * @brief Ends the currently active query.
     * @param InPool Pool that owns the query index.
     * @param InIndex Index matching the active query.
     * @return Success, or InvalidState if no matching query is active.
     */
    virtual TExpected<void, FRHIError> EndQuery ( IRHIQueryPool &InPool, UInt32 InIndex ) noexcept = 0;

    /**
     * @brief Opens a named debug region for diagnostic tooling.
     * @param InName Non-owning name copied by the backend when it records the region.
     * @return Success, or InvalidState outside a recording scope.
     */
    virtual TExpected<void, FRHIError> BeginDebugRegion ( FStringView InName ) noexcept = 0;

    /**
     * @brief Closes the most recently opened debug region.
     * @return Success, or InvalidState if no debug region is open.
     */
    virtual TExpected<void, FRHIError> EndDebugRegion () noexcept = 0;
};
} // namespace Lumen
