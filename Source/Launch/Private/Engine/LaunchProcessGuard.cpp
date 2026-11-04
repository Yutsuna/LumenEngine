/// @file LaunchProcessGuard.cpp
/// @brief Implementation of Launch Process Guard

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/LaunchProcessGuard.hpp>

#include <Core/CoreDefinitions.hpp>

#include <HAL/Signal.hpp>

#include <Logging/Logger.hpp>

namespace
{

/// @brief Handles Signals received
template <typename TMessage>
void OnSignal ( const Lumen::ESystemSignal::Type InSignal, const TMessage &InMessage ) noexcept
{
    if ( Lumen::Launch::FLaunchEngineLoop::RequestExit() )
    {
        Lumen::FLogger::Flush( InMessage );
        Lumen::FSignal::Unbind( InSignal );
        Lumen::FSignal::Raise( InSignal );
    }
}

/// @brief Handles Interrupt Signal
void OnInterruptSignal ( const Lumen::ESystemSignal::Type InSignal ) noexcept
{
    OnSignal( InSignal, "\rInterrupt signal trapped: exiting gracefully..." LUMEN_NEWLINE );
}

/// @brief Handles Terminate Signal
void OnTerminateSignal ( const Lumen::ESystemSignal::Type InSignal ) noexcept
{
    OnSignal( InSignal, "\rTerminate signal trapped: exiting gracefully..." LUMEN_NEWLINE );
}

/// @brief Handles Out Of Memory
void OnOutOfMemory () noexcept
{
    Lumen::FLogger::Flush( "Fatal: Out of Memory" LUMEN_NEWLINE );
    std::_Exit( static_cast<Lumen::Int32>( Lumen::Launch::ELaunchErrorCode::OutOfMemory ) );
}

} // namespace

Lumen::Launch::FLaunchProcessGuard::FLaunchProcessGuard () noexcept : PreviousNewHandler( std::set_new_handler( &OnOutOfMemory ) )
{
    FSignal::Bind( ESystemSignal::Interrupt, &OnInterruptSignal );
    FSignal::Bind( ESystemSignal::Terminate, &OnTerminateSignal );

    FLogger::GetInstance().Initialize();
}

Lumen::Launch::FLaunchProcessGuard::~FLaunchProcessGuard () noexcept
{
    FLogger::GetInstance().Shutdown();

    FSignal::Unbind( ESystemSignal::Terminate );
    FSignal::Unbind( ESystemSignal::Interrupt );

    std::set_new_handler( PreviousNewHandler );
}
