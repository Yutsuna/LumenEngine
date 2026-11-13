/**
 * @file StopToken.hpp
 * @brief Cooperative cancellation primitives.
 */

#pragma once

#include <CoreExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformThread.hpp>
#include <Templates/Invoke.hpp>

#include <type_traits>

namespace Lumen
{

namespace Internal
{

    class FStopState;

    /**
     * @enum ECallbackStatus
     * @brief Lifecycle of a callback node.
     *
     * Transitions:
     *  - Registered -> Pending -> Executing -> Finished : stop requested while registered.
     *  - Registered -> Finished                         : unregistered before any stop request.
     *  - Pending    -> Finished                         : unregistered after detach, before execution.
     *  - Finished                                       : initial and terminal state.
     */
    enum class ECallbackStatus : UInt8
    {
        Registered,
        Pending,
        Executing,
        Finished,
    };

    /**
     * @struct FCallbackNode
     * @brief Type-erased base class for callback nodes.
     * @details Reference-counted with two kinds of owners: the FStopCallback handle owns one
     *          reference, and the stop state owns one while the node is linked or executing.
     *          The executor releases its reference only after the callback has fully returned,
     *          which is what makes destroying a handle from inside its own callback safe.
     */
    struct CORE_EXPORT FCallbackNode
    {
        FCallbackNode () noexcept;
        virtual ~FCallbackNode () noexcept;

        /** @brief Invokes the stored callback. Must not throw. */
        virtual void Execute () noexcept = 0;

        void AddReference () noexcept;
        void ReleaseReference () noexcept;

    private:

        friend class FStopState;

        TAtomic<UInt32> ReferenceCount  = { 1U };
        TAtomic<ECallbackStatus> Status = { ECallbackStatus::Finished };
        FCallbackNode *Previous         = nullptr;
        FCallbackNode *Next             = nullptr;
        HAL::FPlatformThread::FId ExecutingThread{};
    };

} // namespace Internal

/**
 * @class FStopToken
 * @brief Represents a token that can be used to observe a stop request.
 * @details Copyable, movable, cheap to poll: StopRequested() is a lock-free atomic read.
 */
class CORE_EXPORT FStopToken final
{
public:

    /** @brief Creates a token with no associated stop state. */
    FStopToken () noexcept = default;

public:

    FStopToken ( const FStopToken &InOther ) noexcept;
    FStopToken ( FStopToken &&InOther ) noexcept;
    ~FStopToken () noexcept;

    FStopToken &operator=( const FStopToken &InOther ) noexcept;
    FStopToken &operator=( FStopToken &&InOther ) noexcept;

public:

    /** @return True when this token can observe a stop request. */
    [[nodiscard]] Bool StopPossible () const noexcept;

    /** @return True when the associated source has received a stop request. */
    [[nodiscard]] Bool StopRequested () const noexcept;

    void Swap ( FStopToken &InOther ) noexcept;

    [[nodiscard]] Bool operator==( const FStopToken &InOther ) const noexcept;

private:

    explicit FStopToken ( Internal::FStopState *InState ) noexcept;

private:

    Internal::FStopState *State = nullptr;

    friend class FStopSource;
    friend class FStopCallback;
};

/**
 * @class FStopSource
 * @brief Represents a source that can be used to request a stop.
 */
class CORE_EXPORT FStopSource final
{
public:

    /** @brief Creates a source with its own shared stop state. */
    FStopSource () noexcept;

    /** @brief Creates a source without stop state. */
    FStopSource ( NullPtrType ) noexcept;

public:

    FStopSource ( const FStopSource &InOther ) noexcept;
    FStopSource ( FStopSource &&InOther ) noexcept;
    ~FStopSource () noexcept;

    FStopSource &operator=( const FStopSource &InOther ) noexcept;
    FStopSource &operator=( FStopSource &&InOther ) noexcept;

public:

    /** @return A token observing this source. */
    [[nodiscard]] FStopToken GetToken () const noexcept;

    /**
     * @brief Requests cancellation and invokes registered callbacks exactly once.
     * @details Callbacks run synchronously on the calling thread, in reverse registration order,
     *          with the internal mutex released. A callback may register or unregister callbacks,
     *          and may destroy token/source handles (including the source being stopped).
     * @return True only for the call that changes the state to stopped.
     */
    [[nodiscard]] Bool RequestStop () noexcept;

    /** @return True when this source has associated stop state. */
    [[nodiscard]] Bool StopPossible () const noexcept;

    /** @return True after a successful stop request. */
    [[nodiscard]] Bool StopRequested () const noexcept;

    void Swap ( FStopSource &InOther ) noexcept;

    [[nodiscard]] Bool operator==( const FStopSource &InOther ) const noexcept;

private:

    Internal::FStopState *State = nullptr;
};

/**
 * @class FStopCallback
 * @brief Represents a callback that is invoked when a stop request is made.
 * @details The callback is invoked at most once: either by RequestStop(), or inline at
 *          construction time if stop was already requested. The callback must not throw and
 *          must be nothrow-constructible. It may destroy its own handle; in that case the node
 *          is kept alive until execution has fully returned.
 */
class CORE_EXPORT FStopCallback final : public FNonCopyable, public FNonMovable
{
public:

    /**
     * @brief Registers a callback, or invokes it immediately if stop was already requested.
     * @details The callback is destroyed with this handle. Destroying the handle during concurrent
     *          execution waits for completion unless it is destroyed by the executing callback itself.
     *
     * @param InStopToken The stop token to register with.
     * @param InCallback The callback to invoke.
     */
    template <typename TCallback>
        requires CIsInvocable<std::decay_t<TCallback> &>
    explicit FStopCallback( const FStopToken &InStopToken, TCallback &&InCallback ) noexcept;

    ~FStopCallback () noexcept;

private:

    /** @brief Registers the callback with the stop token. */
    void Register ( const FStopToken &InStopToken ) noexcept;

    /** @brief Resets the callback, removing it from the stop token. */
    void Reset () noexcept;

private:

    Internal::FStopState *State       = nullptr;
    Internal::FCallbackNode *Callback = nullptr;
};

} // namespace Lumen

#include "Inline/StopToken.inl"
