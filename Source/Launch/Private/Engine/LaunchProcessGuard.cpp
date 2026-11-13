/**
 * @file LaunchProcessGuard.cpp
 * @brief Implementation of Launch Process Guard
 */

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/LaunchProcessGuard.hpp>

#include <Core/CoreDefinitions.hpp>

#include <HAL/PlatformSignal.hpp>

#include <Logging/Logger.hpp>

namespace
{

/** @brief Handles Signals received */
template <typename TMessage> void OnSignal ( const Lumen::HAL::ESystemSignal::Type InSignal, const TMessage &InMessage ) noexcept
{
    if ( Lumen::Launch::FLaunchEngineLoop::RequestExit() )
    {
        Lumen::FLogger::Flush( InMessage );
        Lumen::HAL::FPlatformSignal::Unbind( InSignal );
        Lumen::HAL::FPlatformSignal::Raise( InSignal );
    }
}

/** @brief Handles Interrupt Signal */
void OnInterruptSignal ( const Lumen::HAL::ESystemSignal::Type InSignal ) noexcept
{
    OnSignal( InSignal, "\rInterrupt signal trapped: exiting gracefully..." LUMEN_NEWLINE );
}

/** @brief Handles Terminate Signal */
void OnTerminateSignal ( const Lumen::HAL::ESystemSignal::Type InSignal ) noexcept
{
    OnSignal( InSignal, "\rTerminate signal trapped: exiting gracefully..." LUMEN_NEWLINE );
}

/** @brief Handles Out Of Memory */
void OnOutOfMemory () noexcept
{
    Lumen::FLogger::Flush( "Fatal: Out of Memory" LUMEN_NEWLINE );
    std::_Exit( static_cast<Lumen::Int32>( Lumen::Launch::ELaunchErrorCode::OutOfMemory ) );
}

} // namespace

Lumen::Launch::FLaunchProcessGuard::FLaunchProcessGuard () noexcept : PreviousNewHandler( std::set_new_handler( &OnOutOfMemory ) )
{
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Interrupt, &OnInterruptSignal );
    HAL::FPlatformSignal::Bind( HAL::ESystemSignal::Terminate, &OnTerminateSignal );

    FLogger::GetInstance().Initialize();
}

Lumen::Launch::FLaunchProcessGuard::~FLaunchProcessGuard () noexcept
{
    FLogger::GetInstance().Shutdown();

    HAL::FPlatformSignal::Unbind( HAL::ESystemSignal::Terminate );
    HAL::FPlatformSignal::Unbind( HAL::ESystemSignal::Interrupt );

    std::set_new_handler( PreviousNewHandler );
}
