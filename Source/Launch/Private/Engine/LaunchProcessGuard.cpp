/// @file LaunchProcessGuard.cpp
/// @brief Implementation of Launch Process Guard

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/LaunchProcessGuard.hpp>

#include <Core/CoreDefinitions.hpp>

#include <HAL/Signal.hpp>

#include <Logging/Logger.hpp>

namespace
{

/// @brief  Handles Signals received
void OnSignal ( const Lumen::ESystemSignal::Type InSignal ) noexcept
{
    if ( Lumen::Launch::FLaunchEngineLoop::RequestExit() )
    {
        Lumen::FSignal::Unbind( InSignal );
        Lumen::FSignal::Raise( InSignal );
    }
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
    FSignal::Bind( ESystemSignal::Interrupt, &OnSignal );
    FSignal::Bind( ESystemSignal::Terminate, &OnSignal );

    FLogger::GetInstance().Initialize();
}

Lumen::Launch::FLaunchProcessGuard::~FLaunchProcessGuard () noexcept
{
    FLogger::GetInstance().Shutdown();

    FSignal::Unbind( ESystemSignal::Terminate );
    FSignal::Unbind( ESystemSignal::Interrupt );

    std::set_new_handler( PreviousNewHandler );
}