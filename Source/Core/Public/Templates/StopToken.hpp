/**
 * @file StopToken.hpp
 * @brief Cooperative cancellation primitives.
 */

#pragma once

#include <CoreExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Templates/Invoke.hpp>

#include <atomic>
#include <thread>
#include <type_traits>

namespace Lumen
{

namespace Internal
{

    class FStopState;

    /**
     * @enum ECallbackStatus
     * @brief Status of a callback node.
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
     * @brief Base class for callback nodes.
     */
    struct CORE_EXPORT FCallbackNode
    {
        FCallbackNode () noexcept;
        virtual ~FCallbackNode () noexcept;

        virtual void Execute () noexcept = 0;

        void AddReference () noexcept;
        void ReleaseReference () noexcept;

        TAtomic<UInt32> ReferenceCount{ 1U };
        TAtomic<ECallbackStatus> Status{ ECallbackStatus::Finished };
        FCallbackNode *Previous = nullptr;
        FCallbackNode *Next     = nullptr;
        FThread::id ExecutingThread{};
    };

} // namespace Internal

/**
 * @class FStopToken
 * @brief Represents a token that can be used to observe a stop request.
 */
class CORE_EXPORT FStopToken final
{
public:

    /** @brief Creates a token with no associated stop state. */
    FStopToken () noexcept = default;

    FStopToken ( const FStopToken &InOther ) noexcept;
    FStopToken ( FStopToken &&InOther ) noexcept;
    ~FStopToken () noexcept;

    FStopToken &operator=( const FStopToken &InOther ) noexcept;
    FStopToken &operator=( FStopToken &&InOther ) noexcept;

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
    FStopSource ( const FStopSource &InOther ) noexcept;
    FStopSource ( FStopSource &&InOther ) noexcept;
    ~FStopSource () noexcept;

    FStopSource &operator=( const FStopSource &InOther ) noexcept;
    FStopSource &operator=( FStopSource &&InOther ) noexcept;

    /** @return A token observing this source. */
    [[nodiscard]] FStopToken GetToken () const noexcept;

    /**
     * @brief Requests cancellation and invokes registered callbacks exactly once.
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
 */
class CORE_EXPORT FStopCallback final : public FNonCopyable, public FNonMovable
{
public:

    /**
     * @brief Registers a callback, or invokes it immediately if stop was already requested.
     * @details The callback is destroyed with this handle. Destroying the handle during concurrent execution waits for
     *          completion unless it is destroyed by the executing callback itself.
     */
    template <typename TCallback>
        requires Concepts::CIsInvocable<std::decay_t<TCallback> &>
    explicit FStopCallback( const FStopToken &InStopToken, TCallback &&InCallback ) noexcept;

    ~FStopCallback () noexcept;

private:

    void Register ( const FStopToken &InStopToken ) noexcept;
    void Reset () noexcept;

private:

    Internal::FStopState *State       = nullptr;
    Internal::FCallbackNode *Callback = nullptr;
};

} // namespace Lumen

#include "Inline/StopToken.inl"
