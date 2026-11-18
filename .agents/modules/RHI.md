# Module: RHI

- **Type:** shared library (`Source/RHI/meson.build`), depends on `Core` and
  `Maths`.
- **Status:** built. The public layers, the Null backend and the validation
  decorator are implemented.
- **Role:** the hardware-independent contract between the renderer and graphics
  backends. RHI defines portable values, interfaces, allocation algorithms,
  validation and test backends. It is backend-neutral: no vendor-, API- or
  window-system-specific implementation belongs in this module.
- **Namespaces and includes:** all RHI API lives in `namespace Lumen {
  namespace RHI { ... } }` (nested blocks, never `namespace Lumen::RHI { }`);
  `.cpp`/`.inl` qualify with `Lumen::RHI::` and leave Core/Maths symbols under
  `Lumen::`. Public headers are under `Public/RHI/` and use `<RHI/...>`
  includes; test helpers are in `Lumen::RHI::Tests`.

## Purpose and scope

The baseline API covers adapter/device creation, graphics/compute/copy queues,
resources and views, memory, bindings, shaders, graphics/compute pipelines,
dynamic rendering, command recording, timeline synchronization, presentation,
queries, caches, debug diagnostics and validation. Optional hardware features
are negotiated through capabilities; a backend must not weaken a required
operation silently.

The public API is a renderer-facing contract. Public base classes and allocator
helpers form an optional backend-facing SPI: backends may use those helpers
without inheriting their allocation or native-handle policy.

## Dependency layers

`Types` depends only on Core and Maths. `Module` and `Device` depend on Types.
Memory value types and requirements precede Resources. Shaders and Queries are
created by Device. Bindings consumes Resources; Pipelines consumes Shaders and
Bindings. Commands consumes Resources, Bindings, Pipelines and Queries.
Synchronization depends on Types. Queue consumes Commands and Synchronization.
Presentation consumes Resources, Queue and Synchronization. Caches consume
immutable descriptors and shared objects. Debug depends on Types. Utilities and
Validation are leaves and are never included by lower layers.

Public headers forward-declare factory return types and references when a
complete type is unnecessary; private implementations may include the complete
type. No lower layer includes a higher layer to reach a concrete backend.
Native surface payloads are isolated in `RHISurfaceDesc.hpp`. Public headers
never include Private or Tests headers. Circular includes are defects even when
a forward declaration would make them compile.

## Object model and ownership

`IRHIObject` is the common polymorphic base: an immutable typed identifier, a
debug name and a backend type tag, with no manual retain/release API.
`FRHIObjectBase` is the exported optional implementation of those operations.
Typed identifiers are values; zero is invalid; uniqueness is per device and
category. They are used by registries, diagnostics and validation, are not
ownership tokens, and are not cast to raw integers in public signatures.

Factories return `TExpected<TSharedRef<T>, FRHIError>`; callers own returned
objects. A device keeps a synchronized non-owning registry and receives
destruction notifications; it never holds a strong reference to every child,
which would form cycles. Resource views and binding sets retain the objects
they reference. A command stream retains every object required by its recorded
commands until the command allocator recycles the stream after GPU completion.
Shared pointers stay out of individual draw/dispatch argument values.

`TryRHIBackendCast` checks the backend tag before casting. The `TSharedRef`
overload returns `TOptional<TSharedPtr<T>>`; the `IRHIObject&` overload returns
a raw pointer or null. It is an explicit extension point, not a replacement for
portable APIs. Validation unwrap checks the decorator chain and the expected
tag. Concrete objects compose `FRHIObjectBase` and use the forwarding macros in
`Private/Types/RHIObjectForward.hpp` (`LUMEN_RHI_OBJECT_DECLARATIONS`,
`LUMEN_RHI_OBJECT_FORWARDERS`); virtual inheritance is forbidden here because
it breaks static downcasts with Core smart pointers.

## Module, device and errors

`IRHIModule` exposes the RHI API version, enumerates immutable
`FRHIAdapterInfo` values and creates a device from `FRHIDeviceDesc`. Adapter
information includes backend identity, queue families, capabilities and limits.
Unsupported requests are rejected before native allocation. `IRHIDevice` is the
factory for all queues, resources, memory objects, binding objects, shaders,
pipelines, command objects, semaphores, swapchains, query pools and caches.
`FRHIDeviceBase` supplies common status, registry and retirement behavior;
native creation stays pure virtual in derived devices.

`FRHIError` carries an `ERHIErrorCode`, a more specific condition, an owned
message and an optional native result. It is a value suitable for monadic
composition. Absence uses `TOptional`; failure uses `TExpected` and
`TUnexpected`. Engine code is non-throwing. Device status is atomic: after loss
or closure, creation and submission fail with `DeviceLost` or `DeviceClosed`;
polling, diagnostics and safe cleanup remain available. No failure path throws
or terminates by default. A missing capability is reported as
`ERHIErrorCode::Unsupported` with `ERHIErrorCondition::Capability`; there is no
`UnsupportedFeature` enumerator.

## Threading model

Module enumeration and device factories are thread-safe unless a concrete
descriptor explicitly documents external ownership. Object identity and debug
name access are synchronized by the object base.

A command list is confined to one recording thread; begin/end, rendering scope
and state changes are externally ordered by that thread. A command allocator
serializes reset and recycling and cannot reset a stream whose submitted list
is pending. Submission to one queue is serialized internally; different queues
may submit concurrently when their synchronization descriptors make resource
access legal. Semaphore values and device status are atomic; timeline values
never regress.

Caches, shared allocators, object registries and deferred release queues are
thread-safe. Binding-set writes are serialized per set by a mutex and tracked
with a single coarse dirty flag (`IsDirty()` / `ClearDirty()`). Validation uses
fine-grained shared locking for read-mostly registries and
exclusive locking for graph changes. Core SPMC queues are not used for MPSC
submission; triple buffers are not used for ownership.

## Memory architecture

`FRHIResourceRequirements` describes size, alignment and acceptable memory
classes; `FRHIAllocationDesc` is the allocator-facing form and `FRHIAllocation`
identifies a placed range and its heap. `IRHIHeap` owns native memory and
placement operations; `IRHIAllocator` owns address-range policy.
`FRHIMemoryAllocator` is a facade that selects heaps and allocators, enforces
`FRHIMemoryBudget`, updates `FRHIAllocatorStats` and notifies observers.

All size, offset, pitch and alignment arithmetic uses checked helpers.
Alignment must be a nonzero power of two. Overflow, exhaustion, invalid free
and incompatible placement are explicit errors.

- `FRHILinearAllocator`: bounded bump allocator with global reset.
- `FRHIRingAllocator`: variable ranges, wraparound and retirement fences;
  completed ranges cannot be overwritten before completion.
- `FRHIPoolAllocator`: size classes backed by Core `FIndiceHeapAllocator` bitmaps.
- `FRHITlsfAllocator`: two-level segregated free lists with split/coalesce; not
  an alias for the Core first-fit allocator.
- `FRHIDescriptorAllocatorBase`: Core `FIndiceAllocator` for contiguous ranges,
  with native pool operations delegated to `IRHIDescriptorPoolBackend`.
- `TRHIObjectPool`: fixed slots and bitmap allocation for CPU objects.

Upload and readback allocators use rings and timeline values; mapping is only
allowed in the matching memory class and lifetime, and readback mapping waits
for or observes completion according to the API contract. Transient batches
carry requirements and lifetimes; two resources may alias only when their live
intervals do not overlap and their requirements are compatible. Defragmentation
produces a validated plan of non-overlapping moves, deferred until source
allocations are no longer used by the GPU.

## Resources, bindings, shaders and pipelines

Buffer and texture descriptors are immutable after creation; usage flags
determine legal copies, bindings, transitions and rendering operations. Texture
views retain their texture and expose a validated format and subresource range.
Samplers are immutable and cacheable.

Binding layouts describe immutable slots, descriptor types, counts, stages and
bindless flags. Sets retain written resources and expose a single dirty flag
(`IsDirty()` / `ClearDirty()`). `FRHIBindingWrite` is a typed variant covering buffers, texture views and
samplers; no untyped native descriptor enters the portable API.

Shader packages own their bytecode and reflection. The versioned codec
validates lengths and checksums without exceptions. Reflection describes
inputs, outputs, bindings, inline constants and compute workgroup size.
Backends advertise accepted shader formats through capabilities.

Pipeline layouts combine binding layouts and inline constant ranges. Graphics
pipelines use dynamic rendering attachment formats, not legacy render-pass or
framebuffer objects. Compute pipelines contain only compute state. Dynamic
states not provided at creation must be set during recording.
`FRHIPipelineCompatibility` canonicalizes data affecting pipeline/set
compatibility. RHI hashes hash values, not object addresses, debug names or
uninitialized padding; `FHash::TypeHash` is reserved for backend type tags.

## Commands, synchronization, presentation and queries

`FRHICommandStream` stores aligned opcodes and trivially copyable payloads in
chunked storage and owns required shared references. The Null backend and
validation use it; native backends may record directly while preserving the
same state and lifetime contracts.

A command list moves through `Initial`, `Recording`, `Rendering`, `Executable`
and `Pending` (`ERHICommandListState`). Illegal nesting, draw outside rendering,
reset while pending or submission in a non-executable state is an error. Copy
descriptors validate format aspects, footprints, extents, pitches and
overlapping ranges; transitions validate resource states, subresources and
queue transfers.

Semaphores are binary or timeline (`ERHISemaphoreType`); timeline signals never
regress and binary signals do not form monotonic timelines. They connect queue
submissions, frame completion, allocator recycling, query availability and
presentation. Host waits return success, timeout or a structured error.
`FRHIDeferredReleaseQueue` accepts thread-safe retirement entries tagged with a
timeline value, uses fixed-capacity chunks with Core index allocation,
poll-destroys only completed entries, drains safely at shutdown and owns
deferred callback state until invocation.

Swapchains expose capabilities and a bounded image ring. Acquire returns an
image index, texture, synchronization primitive and status; present validates
the acquired frame and queue synchronization. Out-of-date and suboptimal are
explicit results. Resize/recreate may reference an old swapchain but never
invalidates an image still in flight. Query pools support reset, recording,
resolution and readback after timeline completion; optional timestamp,
occlusion and statistics operations are gated by queue capabilities.

## Caches and diagnostics

Object caches are synchronized templates with explicit ownership and eviction
policies. Pipeline caches use canonical descriptor/compatibility keys, expose
statistics and support versioned opaque serialization; creation faults are
propagated without partial entries.

`FRHIDebugMessage` owns its text and carries severity, category, stable code,
object identifiers and submission context. `IRHIDebugSink` receives messages
without requiring the Core logger; private log helpers adapt messages to
`FLogger`, and validation and tests may use independent sinks.
`FRHIScopedDebugRegion` closes a command region through RAII and never relies
on exception unwinding.

## Validation

Validation is an optional decorator created from `FRHIValidationDesc`. It wraps
a device and every child requiring checks, forwards successful operations,
preserves observable results, reports through `IRHIDebugSink` and returns
structured errors before forwarding an invalid operation. Shared validation
state tracks identity, ownership, resource states, bindings, command states,
queue submissions, swapchain frames, transient lifetimes and deferred
destruction; it detects leaks and invalid unwraps. It has no vendor or platform
dependency and may wrap any conforming backend.

## Null backend and conformance

The Null backend lives only in Tests. It implements every public interface,
owns deterministic simulated data and uses `FNullFaultPlan`
(`Tests/Device/NullFaultPlan.hpp`) to inject an error at a selected operation
and occurrence. It exposes manual timeline advancement so completion and
retirement tests never depend on GPU timing.

`IRHIConformanceDeviceFactory` abstracts device creation for conformance
scenarios. Conformance translation units use only that factory and public RHI
interfaces; `Lumen::RHI::Tests::GetRHIConformanceDeviceFactory()` supplies the Null factory,
and a future backend test target can compile the same translation units with
its own factory. Core scenarios are mandatory; optional scenarios run only when
the corresponding capability is advertised. Conformance checks functional
results, errors, ordering, lifetime and documented concurrency, never native
handles or performance timings, and runs with validation enabled to prove
decorator transparency. `RHI*TierBTests.cpp` cover behavior that a real backend
must implement; the Null backend exercises the portable error paths.

## Testing

`RHITests` is the single RHI test binary. Meson discovers test `.cpp` files at
configure time, so reconfigure after adding or removing one and build
`RHITests` explicitly. Run the suite with `meson test -C Build --suite RHI`.

Deterministic fuzz tests use literal seeds recorded in the test source (not
emitted at runtime) and a simple reference model; stress
tests are bounded and run under sanitizers. Sanitizer builds are configured
manually with `-Db_sanitize=address,undefined` (Debug ASan/UBSan) or
`-Db_sanitize=thread` (TSan); see `commands/build-debug.md` and
`skills/debug-sanitizer.md`.

## Backend porting contract

1. Implement and version `IRHIModule`; enumerate adapters; publish accurate
   capabilities and limits.
2. Implement `IRHIDevice`, optionally deriving `FRHIDeviceBase` and using
   `FRHIObjectBase` for common identity and retirement behavior.
3. Map formats, resource descriptors, memory classes, queues, resource states
   and shader formats without exposing vendor-specific types publicly.
4. Implement heap, descriptor-pool, semaphore, queue and command contracts;
   reuse public allocators and command/descriptor bases where appropriate.
5. Retain recorded references and connect every native destruction to the
   deferred release timeline.
6. Implement presentation only behind a native surface payload and advertise
   the actual capabilities.
7. Supply a conformance device factory and compile the shared scenarios.
8. Run the complete suite with and without validation, then under the
   sanitizers supported by the platform.

A backend-specific workaround must not change portable semantics: if an
operation cannot be represented, capability negotiation rejects it or the
common contract is revised explicitly. Platform conditionals stay in the
backend module.

## Constraints and limitations

RHI depends only on Core and Maths; no allocator, shader-translation or other
third-party dependency belongs here. No vendor name, native header, shader
language or window-system name appears under `Source/RHI`.

Only the Null and validation backends exist; no real graphics backend is
implemented or claimed to be validated. Optional shader formats, query types
and presentation results are supplied or gated by the conformance factory, so a
real backend must validate them independently. Fuzz determinism holds for the
recorded seed values and is not a collision-freedom proof. Inline and template
implementations live in neighboring `Inline/` directories.
