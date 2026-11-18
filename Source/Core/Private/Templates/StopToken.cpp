/**
 * @file StopToken.cpp
 * @brief Implementation of cooperative cancellation primitives.
 */

#include <Templates/StopToken.hpp>

#include <HAL/PlatformMutex.hpp>

#include <new>
#include <utility>

/**
 * Private
 */

/**
 * @class FStopState
 * @brief Shared stop state: a stop flag plus an intrusive list of callback nodes.
 */
class Lumen::Internal::FStopState final
{
public:

    void AddReference () noexcept
    {
        ReferenceCount.fetch_add( 1U, std::memory_order::relaxed );
    }

    void ReleaseReference () noexcept
    {
        if ( ReferenceCount.fetch_sub( 1U, std::memory_order::acq_rel ) == 1U )
        {
            delete this;
        }
    }

    void RegisterCallback ( FCallbackNode &InCallback ) noexcept
    {
        Mutex.Lock();
        if ( not bStopRequested.load( std::memory_order::acquire ) )
        {
            InCallback.AddReference();
            InCallback.Next = Callbacks;
            if ( Callbacks != nullptr )
            {
                Callbacks->Previous = &InCallback;
            }
            Callbacks = &InCallback;
            InCallback.Status.store( ECallbackStatus::Registered, std::memory_order::release );
            Mutex.Unlock();
            return;
        }

        /**
         * INFO: Stop was already requested: run the callback inline, but through the same status
         *       machinery as the deferred path. This publishes ExecutingThread and lets a concurrent
         *       handle destruction wait for completion instead of freeing the node mid-execution.
         *       The extra reference plays the role of the list reference for this execution.
         */
        InCallback.AddReference();
        InCallback.Status.store( ECallbackStatus::Pending, std::memory_order::release );
        Mutex.Unlock();

        /** INFO: The callback may destroy the last token/source handle, so keep the state alive. */
        AddReference();
        ExecuteCallback( InCallback );
        InCallback.ReleaseReference();
        ReleaseReference();
    }

    void UnregisterCallback ( FCallbackNode &InCallback ) noexcept
    {
        Mutex.Lock();
        ECallbackStatus Observed = InCallback.Status.load( std::memory_order::acquire );
        if ( Observed == ECallbackStatus::Registered )
        {
            if ( InCallback.Previous != nullptr )
            {
                InCallback.Previous->Next = InCallback.Next;
            }
            else
            {
                Callbacks = InCallback.Next;
            }
            if ( InCallback.Next != nullptr )
            {
                InCallback.Next->Previous = InCallback.Previous;
            }
            InCallback.Status.store( ECallbackStatus::Finished, std::memory_order::release );
            Mutex.Unlock();

            /**
             * INFO: Unlinked before any stop request could detach the node: no executor can ever
             *       touch it again, so there is nothing to wait for.
             */
            InCallback.ReleaseReference();
            return;
        }
        Mutex.Unlock();

        if ( Observed == ECallbackStatus::Pending )
        {
            /**
             * INFO: Detached by RequestStop but not executing yet: try to cancel it. On success the
             *       executor's CAS fails and the executor releases the list reference itself.
             *       On failure Observed is updated to the executor's state (Executing or Finished).
             */
            if ( InCallback.Status.compare_exchange_strong( Observed, ECallbackStatus::Finished, std::memory_order::acq_rel,
                                                            std::memory_order::acquire ) )
            {
                return;
            }
        }

        while ( Observed == ECallbackStatus::Executing )
        {
            /** INFO: Self-destruction from inside the running callback must not wait on itself. */
            if ( InCallback.ExecutingThread == HAL::FPlatformThread::GetCurrentId() )
            {
                return;
            }
            InCallback.Status.wait( Observed, std::memory_order::acquire );
            Observed = InCallback.Status.load( std::memory_order::acquire );
        }
    }

    [[nodiscard]] Bool RequestStop () noexcept
    {
        if ( bStopRequested.exchange( true, std::memory_order::acq_rel ) )
        {
            return false;
        }

        /**
         * INFO: Callbacks may destroy the last source/token handle, including the very source this
         *       method was called on. Hold a local reference so the state outlives the walk.
         */
        AddReference();

        Mutex.Lock();
        FCallbackNode *Callback = Callbacks;
        Callbacks               = nullptr;
        for ( FCallbackNode *Current = Callback; Current != nullptr; Current = Current->Next )
        {
            Current->Previous = nullptr;
            Current->Status.store( ECallbackStatus::Pending, std::memory_order::release );
        }
        Mutex.Unlock();

        while ( Callback != nullptr )
        {
            FCallbackNode *Next = Callback->Next;
            ExecuteCallback( *Callback );
            Callback->ReleaseReference();
            Callback = Next;
        }

        ReleaseReference();
        return true;
    }

private:

    static void ExecuteCallback ( FCallbackNode &InCallback ) noexcept
    {
        /**
         * INFO: ExecutingThread is written before the CAS so the release side of the CAS publishes
         *       it: a waiter that acquire-loads Executing is guaranteed to see the thread id.
         *       There is exactly one executor per node, so a failed CAS cannot race with a reader.
         */
        InCallback.ExecutingThread = HAL::FPlatformThread::GetCurrentId();

        ECallbackStatus Expected = ECallbackStatus::Pending;
        if ( not InCallback.Status.compare_exchange_strong( Expected, ECallbackStatus::Executing, std::memory_order::acq_rel,
                                                            std::memory_order::acquire ) )
        {
            /** INFO: UnregisterCallback cancelled the node first; it must not execute. */
            return;
        }

        InCallback.Execute();
        InCallback.Status.store( ECallbackStatus::Finished, std::memory_order::release );
        InCallback.Status.notify_all();
    }

private:

    TAtomic<UInt32> ReferenceCount{ 1U };
    TAtomic<Bool> bStopRequested{ false };
    HAL::FPlatformMutex Mutex;
    FCallbackNode *Callbacks = nullptr;

    friend class Lumen::FStopSource;
    friend class Lumen::FStopToken;
};

Lumen::Internal::FCallbackNode::FCallbackNode () noexcept  = default;
Lumen::Internal::FCallbackNode::~FCallbackNode () noexcept = default;

void Lumen::Internal::FCallbackNode::AddReference () noexcept
{
    ReferenceCount.fetch_add( 1U, std::memory_order::relaxed );
}

void Lumen::Internal::FCallbackNode::ReleaseReference () noexcept
{
    if ( ReferenceCount.fetch_sub( 1U, std::memory_order::acq_rel ) == 1U )
    {
        delete this;
    }
}

Lumen::FStopToken::FStopToken ( const FStopToken &InOther ) noexcept : State( InOther.State )
{
    if ( State != nullptr )
    {
        State->AddReference();
    }
}

Lumen::FStopToken::FStopToken ( FStopToken &&InOther ) noexcept : State( std::exchange( InOther.State, nullptr ) )
{
    /* */
}

Lumen::FStopToken::~FStopToken () noexcept
{
    if ( State != nullptr )
    {
        State->ReleaseReference();
    }
}

Lumen::FStopToken &Lumen::FStopToken::operator=( const FStopToken &InOther ) noexcept
{
    if ( this != &InOther )
    {
        if ( InOther.State != nullptr )
        {
            InOther.State->AddReference();
        }
        if ( State != nullptr )
        {
            State->ReleaseReference();
        }
        State = InOther.State;
    }
    return *this;
}

Lumen::FStopToken &Lumen::FStopToken::operator=( FStopToken &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        if ( State != nullptr )
        {
            State->ReleaseReference();
        }
        State = std::exchange( InOther.State, nullptr );
    }
    return *this;
}

Lumen::Bool Lumen::FStopToken::StopPossible () const noexcept
{
    return State != nullptr;
}

Lumen::Bool Lumen::FStopToken::StopRequested () const noexcept
{
    return State != nullptr and State->bStopRequested.load( std::memory_order::acquire );
}

void Lumen::FStopToken::Swap ( FStopToken &InOther ) noexcept
{
    std::swap( State, InOther.State );
}

Lumen::Bool Lumen::FStopToken::operator==( const FStopToken &InOther ) const noexcept
{
    return State == InOther.State;
}

Lumen::FStopToken::FStopToken ( Internal::FStopState *InState ) noexcept : State( InState )
{
    if ( State != nullptr )
    {
        State->AddReference();
    }
}

Lumen::FStopSource::FStopSource () noexcept : State( new ( std::nothrow ) Internal::FStopState() )
{
}

Lumen::FStopSource::FStopSource ( NullPtrType ) noexcept
{
}

Lumen::FStopSource::FStopSource ( const FStopSource &InOther ) noexcept : State( InOther.State )
{
    if ( State != nullptr )
    {
        State->AddReference();
    }
}

Lumen::FStopSource::FStopSource ( FStopSource &&InOther ) noexcept : State( std::exchange( InOther.State, nullptr ) )
{
}

Lumen::FStopSource::~FStopSource () noexcept
{
    if ( State != nullptr )
    {
        State->ReleaseReference();
    }
}

Lumen::FStopSource &Lumen::FStopSource::operator=( const FStopSource &InOther ) noexcept
{
    if ( this != &InOther )
    {
        if ( InOther.State != nullptr )
        {
            InOther.State->AddReference();
        }
        if ( State != nullptr )
        {
            State->ReleaseReference();
        }
        State = InOther.State;
    }
    return *this;
}

Lumen::FStopSource &Lumen::FStopSource::operator=( FStopSource &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        if ( State != nullptr )
        {
            State->ReleaseReference();
        }
        State = std::exchange( InOther.State, nullptr );
    }
    return *this;
}

Lumen::FStopToken Lumen::FStopSource::GetToken () const noexcept
{
    return FStopToken( State );
}

Lumen::Bool Lumen::FStopSource::RequestStop () noexcept
{
    return State != nullptr and State->RequestStop();
}

Lumen::Bool Lumen::FStopSource::StopPossible () const noexcept
{
    return State != nullptr;
}

Lumen::Bool Lumen::FStopSource::StopRequested () const noexcept
{
    return State != nullptr and State->bStopRequested.load( std::memory_order::acquire );
}

void Lumen::FStopSource::Swap ( FStopSource &InOther ) noexcept
{
    std::swap( State, InOther.State );
}

Lumen::Bool Lumen::FStopSource::operator==( const FStopSource &InOther ) const noexcept
{
    return State == InOther.State;
}

void Lumen::FStopCallback::Register ( const FStopToken &InStopToken ) noexcept
{
    if ( Callback != nullptr )
    {
        State = InStopToken.State;
        State->AddReference();
        State->RegisterCallback( *Callback );
    }
}

Lumen::FStopCallback::~FStopCallback () noexcept
{
    Reset();
}

void Lumen::FStopCallback::Reset () noexcept
{
    Internal::FCallbackNode *CallbackToRemove = std::exchange( Callback, nullptr );
    Internal::FStopState *CallbackState       = std::exchange( State, nullptr );

    if ( CallbackState != nullptr )
    {
        CallbackState->UnregisterCallback( *CallbackToRemove );
        CallbackToRemove->ReleaseReference();
        CallbackState->ReleaseReference();
    }
}
