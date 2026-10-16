/// @file Signal.cpp
/// @brief Implementation of signal handling functions.

#include <HAL/Signal.hpp>

Lumen::TMap<Lumen::ESystemSignal::Type, Lumen::TVector<Lumen::FSignalHandlerSignature>> Lumen::FSignal::SignalListeners;

void Lumen::FSignal::Bind ( const ESystemSignal::Type InSignal, const FSignalHandlerSignature InHandler )
{
    const Int32 OSCode = InSignal;

    if ( SignalListeners[InSignal].empty() )
    {
        std::signal( OSCode, &FSignal::HandleOSSignal );
    }
    SignalListeners[InSignal].push_back( InHandler );
}

void Lumen::FSignal::Raise ( const ESystemSignal::Type InSignal )
{
    const Int32 OSCode = InSignal;

    std::raise( OSCode );
}

void Lumen::FSignal::Unbind ( const ESystemSignal::Type InSignal )
{
    const Int32 OSCode = InSignal;

    if ( SignalListeners.contains( InSignal ) )
    {
        std::signal( OSCode, SIG_DFL );
        SignalListeners.erase( InSignal );
    }
}

void Lumen::FSignal::Reset ()
{
    for ( const auto &[Signal, Handlers] : SignalListeners )
    {
        static_cast<void>( Handlers );
        const Int32 OSCode = Signal;
        std::signal( OSCode, SIG_DFL );
    }

    SignalListeners.clear();
    SignalListeners.rehash( 0 );
}

void Lumen::FSignal::HandleOSSignal ( const Int32 InSignal )
{
    if ( const ESystemSignal::Type SignalType = static_cast<ESystemSignal::Type>( InSignal );
         SignalListeners.contains( SignalType ) )
    {
        for ( const FSignalHandlerSignature &Handler : SignalListeners[SignalType] )
        {
            Handler( SignalType );
        }
    }
}
