---
name: Core Abstractions
description: How and when to use Lumen Core's Algo algorithms, TOptional, TExpected/TUnexpected, FMonadic and the LUMEN_* propagation macros — before/after cookbook for every important case, plus what cannot be simplified without a cost
---

# Core abstractions: before → after

Companion to `rules/core-first.md` (which Core types are mandatory, and the
full std → Core map), `rules/coding-style.md` (naming, `noexcept`, file
layout) and `modules/Core.md` (what Core offers).

**How to read this page.** Each section shows the code to avoid and the Lumen
form to write instead. A file path after "After" points to an existing usage
in the repository — open it for the surrounding context. Naming, typedefs and
type conventions are those of `rules/coding-style.md`.

Why bother: use the Lumen names throughout. Some of these types are
hand-written in Core (`TOptional`, `TExpected`, `FMonadic`, `Algo`), while
others (`TVector`, `TMap`, `TSpan`, …) are currently aliases over `std`; the
Lumen name is mandatory either way (`rules/core-first.md`, `modules/Core.md`),
so call sites stay consistent when an alias is replaced by a real
implementation. Optimisation lands in Core, so every call site using them
improves at once.

---

# 1. Loop → `Algo` cookbook

## 1.1 `Algo::ForEach` / `Algo::AnyOf` API surface

| Signature (all `constexpr`, `LUMEN_FORCE_INLINE`, in `Lumen::Algo`) | Result | Constraint |
| --- | --- | --- |
| `ForEach( Range, Func )` | `void` | none on `Func` result |
| `ForEach( Range, Predicate, Func )` | `void` | `Predicate` result → `Bool` |
| `ForEachUntil( Range, Func )` | pointer to first matching element, or `nullptr` | `Func` result → `Bool` |
| `ForEachUntil( Range, Predicate, Func )` | `Bool` (true = stopped early) | both results → `Bool` |
| `AnyOf( Range, Predicate )` | `Bool` | `Predicate` result → `Bool` |

Headers: `<Algo/ForEach.hpp>`, `<Algo/Any.hpp>`. Callables are bound once
outside the loop with `auto &&Callable = std::forward<TFunc>( Func )`, so an
rvalue lambda is not moved-from on every iteration. `Invoke` supports free
functions, member pointers and lambdas.

## 1.2 Plain `for` → `ForEach( Range, Func )`

Before:

```cpp
for ( const FRecord &Record : Records )
{
    Process( Record );
}
```

After:

```cpp
Algo::ForEach( Records, [] ( const FRecord &InRecord ) noexcept
{
    Process( InRecord );
} );
```

## 1.3 `for` + `if` → `ForEach( Range, Predicate, Func )`

Before:

```cpp
for ( FRecord &Record : Records )
{
    if ( Record.bReady )
    {
        Process( Record );
    }
}
```

After:

```cpp
Algo::ForEach( Records, [] ( const FRecord &InRecord ) noexcept -> Bool
{
    return InRecord.bReady;
}, [] ( FRecord &InRecord ) noexcept
{
    Process( InRecord );
} );
```

Predicate comes **first**, action second. One pass, one predicate check per
element — identical to the manual loop.

## 1.4 `for` + `if` + `break` + pointer → `ForEachUntil( Range, Func )`

Before:

```cpp
FRecord *Found = nullptr;
for ( FRecord &Record : Records )
{
    if ( Record.bReady )
    {
        Found = &Record;
        break;
    }
}
```

After:

```cpp
if ( FRecord *Found = Algo::ForEachUntil( Records, [] ( const FRecord &InRecord ) noexcept -> Bool
{
    return InRecord.bReady;
} ) )
{
    Found->Process();
}
```

Returns a pointer into the range (or `nullptr`). The range must satisfy
`Concepts::CBorrowedRange` (an lvalue or a view marked `EnableBorrowedRangeV`),
and its reference type must be a real reference; temporary owning ranges and
proxy ranges are rejected at compile time. For an extracted value instead, use `FindIfOrNone( Range, Predicate )`
→ `TOptional<ElementValue>`.

## 1.5 `for` + `if` + action + early `return true` → `ForEachUntil( Range, Pred, Func )`

Before:

```cpp
for ( FRecord &Record : Records )
{
    if ( Record.bReady and TryProcess( Record ) )
    {
        return true;
    }
}
return false;
```

After:

```cpp
return Algo::ForEachUntil( Records, [] ( const FRecord &InRecord ) noexcept -> Bool
{
    return InRecord.bReady;
}, [] ( FRecord &InRecord ) noexcept -> Bool
{
    return TryProcess( InRecord );
} );
```

Implemented via `AnyOf` with `Predicate( Item ) and Func( Item )`: single
pass, short-circuits, `Func` is not called when `Predicate` is false. No extra
branch compared with the manual loop.

## 1.6 "Any match?" → `AnyOf`

Before:

```cpp
for ( const ERHIShaderFormat Format : Required.ShaderFormats )
{
    if ( not Adapter.Capabilities.Supports( Format ) ) { return true; }
}
return false;
```

After (`RHI/Private/Device/RHIBackendSupport.cpp`):

```cpp
if ( Algo::AnyOf( Required.ShaderFormats, [&InAdapter] ( const ERHIShaderFormat Format )
{
    return not InAdapter.Capabilities.Supports( Format );
} ) )
{
    return MakeUnexpected(
        MakeRHIUnsupported( ERHIErrorCondition::Shader, "Adapter does not provide a required shader format" ) );
}
```

"All match?" has no `AllOf` in Core; De Morgan in one pass:

```cpp
const Bool bAllReady = not Algo::AnyOf( Records, [] ( const FRecord &InRecord ) noexcept -> Bool
{
    return not InRecord.bReady;
} );
```

Use only when the negated predicate has no side effects you depend on.

## 1.7 Search → `Find*` / `Find*OrNone`

Before:

```cpp
for ( const FWindow &Window : Windows )
{
    if ( Window.GetOSWindowID() == Id ) { return Window; }
}
return {};
```

After (`AppCore/Private/Linux/LinuxApplication.cpp`):

```cpp
TOptional<TSharedPtr<FLinuxWindow>> FindWindowByID ( const SDL_WindowID InWindowID ) const noexcept
{
    return Algo::FindIfOrNone( Windows, [&] ( const TSharedRef<FLinuxWindow> &Window ) noexcept -> Bool
    {
        return Window->GetOSWindowID() == InWindowID;
    } );
}
```

`FindIf`, `Find`, `FindIfNot`, `FindIfEqual` exist for iterator pairs **and**
ranges; the `…OrNone` variants return `TOptional<Iterator>` (iterator form) or
`TOptional<ElementValue>` (range form). The element is copied from an lvalue
or borrowed range and moved only from a non-borrowed rvalue range. Use
iterator/pointer overloads when you need identity or mutation.

`Find( First, Last, Value )` has a specialized runtime path for contiguous
integral elements when the probe has the exact same type: byte searches use
`memchr`; x86-64 searches on 16/32/64-bit elements use SSE2 with a scalar tail.
Constant evaluation and other types keep the portable scalar loop. `FindIf`
never speculatively invokes predicates beyond the first match.

## 1.8 Sort / equality

Before:

```cpp
std::sort( Sorted.begin(), Sorted.end(), [] ( const FRHIAllocation &A, const FRHIAllocation &B ) { return A.Offset < B.Offset; } );
Bool bSame = std::equal( A.begin(), A.end(), B.begin(), B.end() );
```

After (`RHI/Private/Memory/RHIDefragmenter.cpp` for the sort):

```cpp
Algo::StableSort( Sorted, [] ( const FRHIAllocation &InLeft, const FRHIAllocation &InRight ) noexcept
{
    return InLeft.Offset < InRight.Offset;
} );
const Bool bSame = Algo::Equal( RangeA, RangeB );           // Algo/Equal.hpp
const Bool bClose = Algo::Equal( RangeA, RangeB,           // binary predicate overload
    [] ( const FVec InA, const FVec InB ) noexcept -> Bool { return Equals( InA, InB ); } );
```

`Sort` is in-place allocation-free introsort — prefer it. `StableSort` costs
O(n) memory. `Algo::Equal` is a single pass over both ranges and checks that
both ended, so no separate `size()` comparison is needed.

---

# 2. `TOptional` cookbook

Include `<Templates/Optional.hpp>`.

## 2.1 Create / check / access / mutate

Before:

```cpp
std::optional<FString> Name;
if ( Name.has_value() ) { Use( *Name ); }
Name.reset();
Name.emplace( "New" );
```

After:

```cpp
TOptional<FString> Name;                         // empty
if ( Name ) { Use( *Name ); }                    // operator Bool / HasValue()
Name.Reset();
FString &Slot = Name.Emplace( "New" );           // constructs in place, returns ref
TOptional<FString> Copy( "Value" );              // value ctor is explicit
```

`operator*` forwards the value category without copying; `operator->` returns
the real payload address. Both check engagement with `LUMEN_ASSERT` (Debug trap,
removed under `NDEBUG`). Copy/move assignment is
available for assignable payloads. Trivial payloads make the corresponding
special members trivial, so `TOptional<Int32>` is trivially copyable.
`IsOptionalV<T>` is the trait.

## 2.2 "Missing → fallback" → `ValueOr` / `ValueOrElse`

Before:

```cpp
UInt32 Native = VK_SAMPLE_COUNT_1_BIT;
if ( Mapped.HasValue() ) { Native = *Mapped; }
```

After (`RHIVulkan/Tests/Types/VulkanTypeMappingTests.cpp`):

```cpp
const UInt32 Native = ToVulkanSampleCount( Format ).ValueOr( VK_SAMPLE_COUNT_1_BIT );
```

For an expensive fallback, `ValueOrElse` invokes its zero-argument producer
only when empty:

```cpp
const UInt32 Native = ToVulkanSampleCount( Format ).ValueOrElse( [] () noexcept
{
    return ComputeNativeSampleCount();
} );
```

## 2.3 "Missing → produce a fallback container" → `OrElse` (yes, it works on `TOptional`)

`TOptional` has no `Error()`, so `OrElse` calls the callback with **no
arguments**:

```cpp
TOptional<FString> Resolved = FindOverride( Key )
    .OrElse( [] () noexcept -> TOptional<FString>
    {
        return TOptional<FString>( ComputeDefault( Key ) );
    } );
```

## 2.4 Convert `TOptional<Other>` → `TOptional<T>`

Before:

```cpp
TOptional<FString> Converted;
if ( Source.HasValue() ) { Converted = TOptional<FString>( *Source ); }
```

After (converting copy/move constructors; implicit when `Other` → `T` is
implicit, otherwise `explicit`):

```cpp
TOptional<FString> Converted( Source );              // copy, empty stays empty
TOptional<FBase> Base( std::move( DerivedOptional ) ); // move conversion
```

## 2.5 Early exit on a miss → `LUMEN_OPTIONAL( Var, Expr )`

Before:

```cpp
auto Window = FindWindowByID( Id );
if ( not Window.HasValue() ) { return {}; }
return ( *Window )->GetTitle();
```

After:

```cpp
TOptional<FStringView> GetTitle ( const WindowId InId ) const noexcept
{
    LUMEN_OPTIONAL( Window, FindWindowByID( InId ) );
    return Window->GetTitle();
}
```

Only valid in a function where `return {}` means empty — in practice a
`TOptional`-returning function. In `TExpected<void, E>`, `return {}` means
**success** and would hide the miss. It declares two locals in the enclosing
scope (`Var##_lumenOptional_` and `Var`), so terminate with `;` and do not
reuse the name. Use `_` to discard the value: `LUMEN_OPTIONAL( _, Expr );`.

## 2.6 Optional carries an error → `LUMEN_TRY_OPTIONAL_ERROR( Expr )`

Before:

```cpp
if ( const TOptional<FError> Error = Context->GetUnavailableError( "Submit" ) )
{
    return MakeUnexpected( *Error );
}
```

After (`RHIVulkan/Private/RHIVulkan/Queue/VulkanQueue.cpp`):

```cpp
LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Queue::Submit" ) );
```

Inverted check: **HasValue means error present**. It is a `do { … }
while( 0 )` block (no leaked locals, no runtime loop) and returns
`MakeUnexpected( *Optional )` from the enclosing function.

---

# 3. `TExpected` / `TUnexpected` cookbook

Includes: `<Templates/Expected.hpp>` (value + `void` specialization and the
macros) and `<Templates/Unexpected.hpp>` (`FUnexpect`, `Unexpect`,
`TUnexpected`, `MakeUnexpected`, `IsUnexpectedV`).

## 3.1 Construct success

Before:

```cpp
std::expected<Int32, FError> Good( 42 );
std::expected<void, FError> Done;
```

After:

```cpp
TExpected<Int32, FError> Good( 42 );                 // implicit value ctor
return 42;                                           // implicit conversion in a return
TExpected<void, FError> Done;                        // default ctor = success
return {};                                           // only for TExpected<void, E>
```

## 3.2 Construct failure — every form

Before:

```cpp
return std::unexpected( FError::Invalid );
```

After:

```cpp
return TUnexpected<FError>( FError::Invalid );                       // explicit wrapper
return TUnexpected<FError>( std::in_place, ArgA, ArgB );              // in-place construction of E
return MakeUnexpected( FError::Invalid );                             // helper; DECAYS the error type
return TExpected<Int32, FError>( Unexpect, FError::Invalid );         // tag form (copy)
return TExpected<Int32, FError>( FUnexpect{}, FError::Invalid );      // same, explicit tag
return TExpected<Int32, FError>( Unexpect, FError{ MoveArgs... } );    // tag form (move)
```

`Unexpect` is the `inline constexpr FUnexpect` instance. `MakeUnexpected`
returns `TUnexpected<std::decay_t<E>>`, so construct `TUnexpected<E>`
directly when the exact error type matters. `TExpected` also has a converting
constructor from `TUnexpected<OtherErrorType>` requiring
`CConstructibleFrom<ErrorType, OtherErrorType&&>` — useful for error upcasts.

## 3.3 Check / dereference / read the error

Before:

```cpp
if ( not Result ) { Handle( Result.error() ); return; }
Use( *Result );
```

After:

```cpp
if ( not Result ) [[unlikely]]
{
    Handle( Result.Error() );                                    // lvalue → copy
    return;
}
Use( *Result );                                                  // asserts HasValue()
Result->Member();                                                // arrow overload
return MakeUnexpected( std::forward<decltype( Result )>( Result ).Error() ); // rvalue → move
```

`HasValue()`, `operator Bool`, `*` and `Error()` exist on both specializations;
`->` exists on the value-carrying one. `Error()` asserts the object holds an
error. Forwarding the result before calling `Error()` is what lets the macros
move instead of copy.

For non-trivial payloads, assignment uses a temporary and a non-throwing
`Swap`, giving the destination a strong exception guarantee without
allocation. This requires nothrow move construction, swap and destruction for
both payload types. Trivial payloads keep trivial special members;
`TExpected<Int32, Int32>` and `TExpected<void, Int32>` are trivially copyable.

## 3.4 Propagate another expected's failure → `LUMEN_EXPECT( Var, Expr )`

Before:

```cpp
auto Config = Negotiate( InAdapter, InDesc );
if ( not Config ) { return MakeUnexpected( std::move( Config ).Error() ); }
Config.Features.pNext = ...;
```

After (`RHIVulkan/Private/RHIVulkan/Device/VulkanDevice.cpp`):

```cpp
LUMEN_EXPECT( Config, NegotiateVulkanDeviceFeatures( InAdapter, InDesc ) );
Config.Features12.pNext = &Config.Features13;
```

Chain of validation calls (`RHI/Private/Device/RHIBackendSupport.cpp`):

```cpp
LUMEN_EXPECT( _, InDesc.Validate() );                       // discard a void result
LUMEN_EXPECT( CapabilityValidation, InDesc.RequiredCapabilities.Validate() );
LUMEN_EXPECT( ShaderFormatsCheck, ValidateShaderFormats( InAdapter, InDesc.RequiredCapabilities ) );
return {};
```

On failure it returns `MakeUnexpected( …Error() )`; on success `Var` is an
`auto&&` bound to the unwrapped value (for `TExpected<void, E>` an opaque
placeholder — use the next macro for void results). Branch is marked
`[[unlikely]]`; it is the same branch manual code needs, not an extra one.

## 3.5 Propagate without unwrapping → `LUMEN_TRY_EXPECT_ERROR( Expr )`

Before / after (`RHI/Private/Commands/RHIRenderingDesc.cpp`):

```cpp
// before
auto Color = Attachment.Validate( false );
if ( not Color ) { return MakeUnexpected( std::move( Color ).Error() ); }
auto Depth = DepthStencilAttachment.Validate( true );
if ( not Depth ) { return MakeUnexpected( std::move( Depth ).Error() ); }

// after
LUMEN_TRY_EXPECT_ERROR( Attachment.Validate( false ) );
LUMEN_TRY_EXPECT_ERROR( DepthStencilAttachment.Validate( true ) );
```

`do { … } while( false )` block; no locals leak; use for void-valued
validation results.

## 3.6 Map a value → `.Transform()`

Before:

```cpp
auto Sum = RHICheckedAdd( Value, Alignment - 1U );
TOptional<ValueType> Result;
if ( Sum.HasValue() ) { Result = TOptional<ValueType>( *Sum & ~( Alignment - 1U ) ); }
```

After (`RHI/Public/RHI/Utilities/Inline/RHIAlignment.inl`):

```cpp
return RHICheckedAdd( InValue, static_cast<ValueType>( InAlignment - 1U ) )
    .Transform( [InAlignment] ( const ValueType InPadded ) noexcept -> ValueType
{
    return static_cast<ValueType>( InPadded & ~( InAlignment - 1U ) );
} );
```

`T → U` with `Rebind`: `TExpected<T,E> → TExpected<U,E>`,
`TOptional<T> → TOptional<U>`. Failure/emptiness propagates via
`MonadicFail` / default construction; the callback is skipped.

## 3.7 Chain another fallible step → `.AndThen()`

Before / after (`RHI/Public/RHI/Commands/Inline/RHICommandStream.inl`):

```cpp
// before
auto View = Read( InIndex );
if ( not View ) { return MakeUnexpected( std::move( View ).Error() ); }
if ( View->Opcode != ExpectedOpcode ) { return MakeUnexpected( MakeRHICorrupted( ... ) ); }
PayloadType Payload;
std::memcpy( std::addressof( Payload ), View->Payload.data(), sizeof( PayloadType ) );
return Payload;

// after
return Read( InIndex ).AndThen(
    [InExpectedOpcode] ( const FRHICommandView &InView ) noexcept -> TExpected<PayloadType, FRHIError>
{
    if ( InView.Opcode != InExpectedOpcode or InView.Payload.size() != sizeof( PayloadType ) )
    {
        return TUnexpected<FRHIError>( MakeRHICorrupted( ERHIErrorCondition::Command, "mismatch" ) );
    }
    PayloadType Payload;
    std::memcpy( std::addressof( Payload ), InView.Payload.data(), sizeof( PayloadType ) );
    return Payload;
} );
```

Callback must return a monadic container. Outer and inner failures both
propagate. On `TExpected<void, E>` the callback takes **zero** arguments:

```cpp
return Validate().AndThen( [] () noexcept -> TExpected<UInt32, FError> { return Compute(); } );
```

## 3.8 Recover from an error → `.OrElse()`

Before:

```cpp
auto Result = Load();
if ( not Result ) { Log( Result.Error() ); return MakeFallback(); }
return Result;
```

After (`Launch/Public/CLI/Inline/CommandLine.inl`):

```cpp
return Load()
    .OrElse( [] ( const FError InError ) noexcept -> TExpected<TValue, FError>
    {
        Log( InError );
        return MakeFallback();
    } );
```

The callback receives `Error()` for `TExpected`, no arguments for `TOptional`.
It must return the **same** container type. Prefer it on an rvalue
(`std::move( Result ).OrElse( … )`) so success is moved, not copied.

## 3.9 Observe a success → `.Inspect()`

Before:

```cpp
auto Result = Transition();
if ( Result ) { Completion = Value; }
return Result;
```

After (`RHI/Tests/Commands/NullCommands.cpp`):

```cpp
return RHITransitionCommandList( State, ERHICommandListState::Pending )
    .Inspect( [this, InCompletion] () noexcept
    {
        Completion = InCompletion;
    } );
```

Note the zero-argument callback: `Inspect` on a `void`-valued expected gets no
value. For value types it receives `const T &`. It returns the same container
(lvalue in → lvalue reference out); it is not `[[nodiscard]]`, so discarding
it is fine.

## 3.10 Unwrap with a fallback → `.ValueOr()` / `.ValueOrElse()`

Before:

```cpp
UInt32 Value = Fallback;
if ( Result ) { Value = *Result; }
```

After:

```cpp
const UInt32 Value = Parsed.ValueOr( Fallback );
```

`ValueOrElse( Func )` takes a zero-argument producer and calls it only on
absence/error. Neither operation is available for `void`-valued containers.

## 3.11 Cross-family bridge (no combinator exists)

Before / after:

```cpp
// TOptional → TExpected: synthesize the error explicitly, then start the chain
TOptional<UInt64> Lookup( Key );
if ( not Lookup ) { return MakeUnexpected( FError::NotFound ); }
return TExpected<UInt64, FError>( *Lookup )
    .Transform( ... )
    .AndThen( ... );
```

`AndThen` cannot turn `TOptional` into `TExpected`: there is no error to
propagate on the empty path. Do not fake it with `ValueOr` (that would swallow
the failure) — add the explicit branch.

---

# 4. `FMonadic` reference (all six methods)

| Method | Callback input | Return | Skipped when |
| --- | --- | --- | --- |
| `Transform` | `TValueRef` (`T&&` on rvalue, `T&`/`const T&` on lvalue) | `Rebind<U>` of the source | empty / error |
| `AndThen` | `TValueRef`, or none for `void` values | the callback's monadic container | empty / error |
| `OrElse` | `Error()` for `TExpected`, none for `TOptional` | exact same container type | value present |
| `Inspect` | `const T &`, or none for `void` values | same container (`Self&` for lvalue) | empty / error |
| `ValueOr` | — | unwrapped `T` or fallback | (always evaluates fallback) |
| `ValueOrElse` | — | unwrapped `T` or callback result | value present |

All are `constexpr`; `Transform`, `AndThen`, `OrElse`, `ValueOr`, `ValueOrElse` are
`[[nodiscard]]`; every success path is `[[likely]]`. `noexcept` is computed
from the callbacks — mark them `noexcept`. `Transform` decays reference-return
callbacks to values (no container of references). `Rebind<NewValueType>` is
required on custom containers; hooks in `Templates/MonadicDetails.hpp` are
`static MonadicFail( Src&& )`, `static MonadicEmplace( Func&&, Args&& )`,
`MonadicConsume()` and `Error()`. `TOptional` and `TExpected` construct the
`Transform` result directly in storage. On rvalues, `MonadicConsume` resets
`TOptional`/`TUniquePtr` after the result is built instead of move-constructing
a guard; containers without the hook need no additional operation.
Tests/contract: `Source/Core/Tests/Functional/Templates/MonadicTests.cpp`.

---

# 5. What CANNOT be simplified (and what costs something)

Reported honestly, against the stated definition of slow: an unnecessary
conditional, or a second pass where one pass sufficed.

## 5.1 No extra cost — safe to adopt unconditionally

| Pattern | Why it is free |
| --- | --- |
| `ForEach` / filtered `ForEach` | One range-for, callable bound once, same branch as manual `if`. |
| `ForEachUntil( Range, Pred, Func )` | Delegates to `AnyOf`; `Pred and Func` short-circuits; one pass. |
| `AnyOf` / `Find*` / `Algo::Equal` | One pass, early return where applicable. |
| `LUMEN_EXPECT` / `LUMEN_TRY_*` | One `[[unlikely]]` branch (the one manual code needs); `do/while(false)` is optimized away. Success binds a reference without copying on lvalues and rvalues; failures forward `Error()`. |
| `Transform` / `AndThen` / `Inspect` / `OrElse` on lvalues | One `Has()` check, then direct construction/return; no second pass. |
| `not AnyOf( Range, not Pred )` for "all" | One pass, same predicate calls — no `AllOf` needed. |

## 5.2 Cannot be expressed with the current API — keep a manual loop

| Case | Why | Do |
| --- | --- | --- |
| Need the element **index** | `ForEach` passes only the element; no index overload. | Manual `for ( USize Index… )`. Don't `Find` then search again for the position. |
| Previous/next or adjacent pairs | No adjacency algorithm. | Manual iterator loop. Calling `Find` + `ForEach` over the same range is two passes. |
| Lockstep iteration over two ranges | No `zip`. | Manual two-iterator loop. `ForEach` over one range + lookup in the other adds a search per element (O(n·m)). |
| Early exit that returns an **accumulated value** | `ForEachUntil` returns only a pointer or `Bool`. | Manual loop, or capture an accumulator by reference in the callback (works, but less clear). |
| Nested early exits across two loops | No cross-loop combinator. | Manual loops. A `bFound` flag plus a second pass would be two iterations. |
| Pointer result from a proxy range | Pointer-returning `ForEachUntil`/`Find` reject ranges whose `operator*` does not return a real reference. | Iterator-based algorithms or a manual loop. |
| Change the **error type** while recovering | `OrElse` must return the same container; there is no `MapError`. | Manual branch constructing the new error type. |
| Cross-family chain (`TOptional` → `TExpected`) | No combinator can synthesize an error. | Explicit `if` bridge (§3.11). |
| Void-value `Transform` / `ValueOr` / `ValueOrElse` | Constraints exclude them. | `AndThen` with a zero-arg callback; `Inspect` for side effects. |
| Container of references from `Transform` | Result references are deliberately decayed. | Store a pointer, or restructure the data flow. |

## 5.3 Expressible, but has a measurable cost — measure before using in hot paths

| Case | Cost | Recommendation |
| --- | --- | --- |
| `Find*OrNone` on an **lvalue or borrowed range** | Returns `TOptional<ElementValue>` and copies the element (refcount bump for `TSharedRef`, string copy for `FString`). A non-borrowed rvalue range moves it instead. | Use the pointer overload (`FindIf`/`Find`) if identity suffices; see the real `LinuxApplication` example. |
| `OrElse` on an **lvalue** | Success path copies the container (`CRecoverable` requires constructible-from-Self). | `std::move( Result ).OrElse( … )` when the source can be consumed. |
| `ValueOr( ComputeFallback() )` | Fallback expression is evaluated **eagerly**, even on success. | Use `ValueOrElse( [] { return ComputeFallback(); } )` to produce it only on absence/error. |
| `TOptional::Emplace()` on a known-empty optional | Calls `Reset()`, which checks `bHasValue` — one predictable extra branch. | Acceptable; only bypass if profiling says so. |
| `StableSort` | O(n) auxiliary memory and merge overhead vs `Sort`. | Use `Sort` unless stability is required. |
| `Inspect`/`AndThen` with a throwing callback | `noexcept` is computed from the callback — the chain loses `noexcept`. | Mark callbacks `noexcept`. |

None of the sanctioned patterns introduce an O(n²) algorithm or a second
iteration over the same range. If you find yourself calling two `Algo`
functions over one range to recover information the first discarded (index,
position, both predicate and element), that composition is the regression —
write the single manual pass instead, or add a Core algorithm with tests.

---

# 6. Workflow

1. Classify: absence → `TOptional`, failure → `TExpected<T, E>`; put it in the
   public signature.
2. Map loops to §1, including the `ForEachUntil` overload that matches your
   early-exit shape.
3. Use §2/§3 for creation, propagation and monadic composition; check §5 if
   the tempting form appears in the "cost" table.
4. Include only used headers (`Algo/*.hpp`, `Templates/Optional.hpp`,
   `Templates/Expected.hpp`, `Templates/Unexpected.hpp` when calling
   `MakeUnexpected` directly).
5. `rg` for `std::optional`, `std::expected`, `std::find*`, `std::sort`,
   `std::any_of`, `throw` and sentinel conventions, then follow
   `commands/test.md` (`meson compile -C Build && meson test -C Build`;
   exit **84** = failure).
