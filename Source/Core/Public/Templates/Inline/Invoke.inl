/**
 * @file Invoke.inl
 * @brief Inline implementation of the Invoke template functions.
 */

#pragma once

#include <Templates/Invoke.hpp>

/**
 * Public
 */

/**
 * @brief Invokes a function with the given arguments and returns the result.
 */
template <typename TFunc, typename... TArgs>
constexpr auto Lumen::Invoke ( TFunc &&InFunc, TArgs &&...InArgs )
    -> decltype( static_cast<TFunc &&>( InFunc )( static_cast<TArgs &&>( InArgs )... ) )
{
    return static_cast<TFunc &&>( InFunc )( static_cast<TArgs &&>( InArgs )... );
}

/**
 * @brief Invokes a pointer-to-member-data on an object (dereferencing the object if needed).
 */
template <typename TReturnType, typename TObjectType, typename TTargetType>
constexpr auto Lumen::Invoke ( TReturnType TObjectType::*InMember, TTargetType &&InObject )
    -> decltype( Internal::MaybeDereference( static_cast<TTargetType &&>( InObject ),
                                             static_cast<std::remove_reference_t<TTargetType> *>( nullptr ) ).*
                 InMember )
{
    return Internal::MaybeDereference( static_cast<TTargetType &&>( InObject ), &InObject ).*InMember;
}

/**
 * Private
 */

template <typename TObjectType, typename TTargetType>
constexpr auto Lumen::Internal::MaybeDereference ( TTargetType &&Target, const volatile TObjectType *TargetPtr )
    -> decltype( static_cast<TTargetType &&>( Target ) )
{
    return static_cast<TTargetType &&>( Target );
}

template <typename TObjectType, typename TTargetType>
constexpr auto Lumen::Internal::MaybeDereference ( TTargetType &&Target, ... )
    -> decltype( *static_cast<TTargetType &&>( Target ) )
{
    return *static_cast<TTargetType &&>( Target );
}
