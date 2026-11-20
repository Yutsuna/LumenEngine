/**
 * @file VulkanHandle.inl
 * @brief Vulkan handle inline implementation.
 */

#pragma once

#include "../VulkanHandle.hpp"

/**
 * Public
 */

template <typename THandleType, typename TDeleterType>
inline Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::TVulkanScopedHandle( THandleType InHandle,
                                                                                              TDeleterType InDeleter ) noexcept
    : Handle( InHandle ), Deleter( static_cast<TDeleterType &&>( InDeleter ) )
{
    /* */
}

template <typename THandleType, typename TDeleterType>
inline Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::TVulkanScopedHandle::~TVulkanScopedHandle() noexcept
{
    Reset();
}

template <typename THandleType, typename TDeleterType>
inline Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::TVulkanScopedHandle(
    TVulkanScopedHandle &&InOther ) noexcept
    : Handle( InOther.Handle ), Deleter( static_cast<TDeleterType &&>( InOther.Deleter ) )
{
    InOther.Handle = VK_NULL_HANDLE;
}

template <typename THandleType, typename TDeleterType>
inline Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType> &
Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::operator=( TVulkanScopedHandle &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        Reset();
        Handle         = InOther.Handle;
        Deleter        = static_cast<TDeleterType &&>( InOther.Deleter );
        InOther.Handle = VK_NULL_HANDLE;
    }
    return *this;
}

template <typename THandleType, typename TDeleterType>
inline void Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::Reset ( THandleType InNewHandle ) noexcept
{
    if ( Handle != VK_NULL_HANDLE and Handle != InNewHandle )
    {
        Deleter( Handle );
    }
    Handle = InNewHandle;
}

template <typename THandleType, typename TDeleterType>
inline THandleType *
Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::ResetAndGetAddressOf ( TDeleterType InDeleter ) noexcept
{
    Reset();
    Deleter = static_cast<TDeleterType &&>( InDeleter );
    return &Handle;
}

template <typename THandleType, typename TDeleterType>
inline THandleType Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::Release () noexcept
{
    const THandleType Released = Handle;
    Handle                     = VK_NULL_HANDLE;
    return Released;
}

template <typename THandleType, typename TDeleterType>
inline THandleType Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::Get () const noexcept
{
    return Handle;
}

template <typename THandleType, typename TDeleterType>
inline const TDeleterType &Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::GetDeleter () const noexcept
{
    return Deleter;
}

template <typename THandleType, typename TDeleterType>
inline Lumen::Bool Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::IsValid () const noexcept
{
    return Handle != VK_NULL_HANDLE;
}

template <typename THandleType, typename TDeleterType>
inline Lumen::RHIVulkan::TVulkanScopedHandle<THandleType, TDeleterType>::operator bool () const noexcept
{
    return IsValid();
}
