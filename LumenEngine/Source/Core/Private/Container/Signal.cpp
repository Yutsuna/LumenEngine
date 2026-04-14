/**
 * @file Signal.cpp
 * @brief Implementation of the FSignal class
 */

#include "Container/Signal.hpp"

LumenEngine::TMap<LumenEngine::ESystemSignal::Type, LumenEngine::TVector<LumenEngine::FSignalHandlerSignature>> LumenEngine::FSignal::SignalListeners;

void LumenEngine::FSignal::Bind ( const ESystemSignal::Type InSignal, FSignalHandlerSignature InHandler )
{
    const Int32 OSCode = static_cast<Int32>( InSignal );

    if ( SignalListeners[InSignal].empty() )
    {
        std::signal( OSCode, &FSignal::HandleOSSignal );
    }
    SignalListeners[InSignal].push_back( InHandler );
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

void LumenEngine::FSignal::Reset ()
{
    for ( const auto &[Signal, Handlers] : SignalListeners )
    {
        static_cast<void>( Handlers );
        const Int32 OSCode = static_cast<Int32>( Signal );
        std::signal( OSCode, SIG_DFL );
    }

    SignalListeners.clear();
    SignalListeners.rehash( 0 );
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
