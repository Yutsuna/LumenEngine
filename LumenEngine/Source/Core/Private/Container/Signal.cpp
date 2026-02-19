/**
 * @file Signal.cpp
 * @brief Implementation of the FSignal class
 */

#include "Container/Signal.hpp"

LumenEngine::Atomic<LumenEngine::Bool> LumenEngine::FSignal::bExitRequested{ false };
LumenEngine::FMap<LumenEngine::ESystemSignal::Type, LumenEngine::FVector<LumenEngine::FSignalHandlerSignature>> LumenEngine::FSignal::SignalListeners;

void LumenEngine::FSignal::Bind ( const ESystemSignal::Type InSignal, FSignalHandlerSignature inHandler )
{
    const Int32 OSCode = static_cast<Int32>( InSignal );

    if ( SignalListeners[InSignal].empty() )
    {
        std::signal( OSCode, &FSignal::HandleOSSignal );
    }
    SignalListeners[InSignal].push_back( inHandler );
}

void LumenEngine::FSignal::Raise ( const ESystemSignal::Type InSignal )
{
    const Int32 OSCode = static_cast<Int32>( InSignal );

    std::raise( OSCode );
}

void LumenEngine::FSignal::Unbind ( const ESystemSignal::Type InSignal )
{
    const Int32 OSCode = static_cast<Int32>( InSignal );

    if ( SignalListeners.contains( InSignal ) )
    {
        std::signal( OSCode, SIG_DFL );
        SignalListeners.erase( InSignal );
    }
}

void LumenEngine::FSignal::HandleOSSignal ( const Int32 InSignal )
{
    const ESystemSignal::Type SignalType = static_cast<ESystemSignal::Type>( InSignal );

    if ( SignalListeners.contains( SignalType ) )
    {
        for ( const FSignalHandlerSignature &Handler : SignalListeners[SignalType] )
        {
            Handler( SignalType );
        }
    }
}
