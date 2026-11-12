/**
 * @file StopToken.cpp
 * @brief Implementation of cooperative cancellation primitives.
 */

#include <Templates/StopToken.hpp>

#include <HAL/PlatformMutex.hpp>

#include <new>
#include <utility>

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
        Mutex.Unlock();

        InCallback.Execute();
    }

    void UnregisterCallback ( FCallbackNode &InCallback ) noexcept
    {
        Bool bReleaseListReference = false;
        Mutex.Lock();
        if ( InCallback.Status.load( std::memory_order::acquire ) == ECallbackStatus::Registered )
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
            bReleaseListReference = true;
        }
        Mutex.Unlock();

        if ( bReleaseListReference )
        {
            InCallback.ReleaseReference();
        }

        ECallbackStatus Observed = InCallback.Status.load( std::memory_order::acquire );
        while ( Observed != ECallbackStatus::Finished )
        {
            if ( Observed == ECallbackStatus::Pending )
            {
                if ( InCallback.Status.compare_exchange_weak( Observed, ECallbackStatus::Finished, std::memory_order::acq_rel,
                                                              std::memory_order::acquire ) )
                {
                    break;
                }
            }
            else if ( Observed == ECallbackStatus::Executing )
            {
                if ( InCallback.ExecutingThread == std::this_thread::get_id() )
                {
                    break;
                }
                InCallback.Status.wait( Observed, std::memory_order::acquire );
                Observed = InCallback.Status.load( std::memory_order::acquire );
            }
        }
    }

    [[nodiscard]] Bool RequestStop () noexcept
    {
        if ( bStopRequested.exchange( true, std::memory_order::acq_rel ) )
        {
            return false;
        }

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
        return true;
    }

private:

    static void ExecuteCallback ( FCallbackNode &InCallback ) noexcept
    {
        ECallbackStatus Expected   = ECallbackStatus::Pending;
        InCallback.ExecutingThread = std::this_thread::get_id();
        if ( not InCallback.Status.compare_exchange_strong( Expected, ECallbackStatus::Executing, std::memory_order::acq_rel,
                                                            std::memory_order::acquire ) )
        {
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
    return State != nullptr && State->bStopRequested.load( std::memory_order::acquire );
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
    return State != nullptr && State->RequestStop();
}

Lumen::Bool Lumen::FStopSource::StopPossible () const noexcept
{
    return State != nullptr;
}

Lumen::Bool Lumen::FStopSource::StopRequested () const noexcept
{
    return State != nullptr && State->bStopRequested.load( std::memory_order::acquire );
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
