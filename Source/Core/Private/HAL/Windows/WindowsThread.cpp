/**
 * @file WindowsThread.cpp
 * @brief Implementation of FWindowsThread.
 */

#include <Core/CoreDefinitions.hpp>

#if defined( LUMEN_PLATFORM_WINDOWS )

    #include <HAL/Windows/WindowsThread.hpp>

    #include <utility>

Lumen::HAL::FWindowsThread::FId::FId ( DWORD InId ) noexcept : Value( InId )
{
}

Lumen::Bool Lumen::HAL::FWindowsThread::FId::IsValid () const noexcept
{
    return Value != 0;
}

Lumen::Bool Lumen::HAL::FWindowsThread::FId::operator==( const FId &InOther ) const noexcept
{
    return Value == InOther.Value;
}

Lumen::HAL::FWindowsThread::FWindowsThread ( FWindowsThread &&InOther ) noexcept
    : Thread( InOther.Thread ), ThreadId( InOther.ThreadId )
{
    InOther.Thread   = nullptr;
    InOther.ThreadId = 0;
}

Lumen::HAL::FWindowsThread &Lumen::HAL::FWindowsThread::operator=( FWindowsThread &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        Reset();
        Thread           = InOther.Thread;
        ThreadId         = InOther.ThreadId;
        InOther.Thread   = nullptr;
        InOther.ThreadId = 0;
    }
    return *this;
}

Lumen::HAL::FWindowsThread::~FWindowsThread () noexcept
{
    Reset();
}

Lumen::Bool Lumen::HAL::FWindowsThread::Start ( FEntryPoint InEntryPoint, void *InData ) noexcept
{
    if ( IsJoinable() )
    {
        return false;
    }

    DWORD NewThreadId = 0;
    HANDLE NewThread  = CreateThread( nullptr, 0, InEntryPoint, InData, 0, &NewThreadId );
    if ( NewThread == nullptr )
    {
        return false;
    }

    Thread   = NewThread;
    ThreadId = NewThreadId;
    return true;
}

void Lumen::HAL::FWindowsThread::Join () noexcept
{
    if ( not IsJoinable() )
    {
        return;
    }
    if ( GetCurrentThreadId() == ThreadId )
    {
        Detach();
        return;
    }
    if ( WaitForSingleObject( Thread, INFINITE ) == WAIT_OBJECT_0 )
    {
        Reset();
    }
}

void Lumen::HAL::FWindowsThread::Detach () noexcept
{
    Reset();
}

void Lumen::HAL::FWindowsThread::Swap ( FWindowsThread &InOther ) noexcept
{
    std::swap( Thread, InOther.Thread );
    std::swap( ThreadId, InOther.ThreadId );
}

Lumen::Bool Lumen::HAL::FWindowsThread::IsJoinable () const noexcept
{
    return Thread != nullptr;
}

Lumen::HAL::FWindowsThread::FId Lumen::HAL::FWindowsThread::GetId () const noexcept
{
    return FId( ThreadId );
}

Lumen::HAL::FWindowsThread::FNativeHandle Lumen::HAL::FWindowsThread::GetNativeHandle () const noexcept
{
    return Thread;
}

Lumen::HAL::FWindowsThread::FId Lumen::HAL::FWindowsThread::GetCurrentId () noexcept
{
    return FId( ::GetCurrentThreadId() );
}

Lumen::UInt32 Lumen::HAL::FWindowsThread::HardwareConcurrency () noexcept
{
    SYSTEM_INFO SystemInfo{};
    GetSystemInfo( &SystemInfo );
    return static_cast<UInt32>( SystemInfo.dwNumberOfProcessors );
}

void Lumen::HAL::FWindowsThread::Yield () noexcept
{
    static_cast<void>( SwitchToThread() );
}

void Lumen::HAL::FWindowsThread::Reset () noexcept
{
    if ( Thread != nullptr )
    {
        static_cast<void>( CloseHandle( Thread ) );
    }
    Thread   = nullptr;
    ThreadId = 0;
}

#endif
