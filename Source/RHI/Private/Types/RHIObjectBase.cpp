/**
 * @file RHIObjectBase.cpp
 * @brief Implements common RHI object identity and debug naming.
 */

#include <HAL/LockGuard.hpp>
#include <Types/RHIObjectBase.hpp>

Lumen::FRHIObjectBase::FRHIObjectBase ( const FRHIObjectId InId, const UInt64 InBackendTypeTag ) noexcept
    : Id( InId ), BackendTypeTag( InBackendTypeTag )
{
    /* */
}

Lumen::FRHIObjectBase::~FRHIObjectBase () noexcept
{
    if ( DestructionCallback )
    {
        DestructionCallback();
    }
}

void Lumen::FRHIObjectBase::SetDestructionCallback ( TFunction<void()> InCallback ) noexcept
{
    DestructionCallback = std::move( InCallback );
}

Lumen::FString Lumen::FRHIObjectBase::GetDebugName () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( DebugNameMutex );
    return DebugName;
}

void Lumen::FRHIObjectBase::SetDebugName ( const FStringView InName ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( DebugNameMutex );
    DebugName = InName;
}
