/**
 * @file Invoke.hpp
 * @brief Invokes a function on an object.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#include <type_traits>
#include <utility>

namespace Lumen
{

namespace Internal
{

    /**
     * INFO: Declared before Invoke because the pointer-to-member Invoke overload
     *       uses it in its trailing return type.
     */
    template <typename TObjectType, typename TTargetType>
    constexpr auto MaybeDereference ( TTargetType &&Target, const volatile TObjectType *TargetPtr )
        -> decltype( static_cast<TTargetType &&>( Target ) );

    template <typename TObjectType, typename TTargetType>
    constexpr auto MaybeDereference ( TTargetType &&Target, ... ) -> decltype( *static_cast<TTargetType &&>( Target ) );

} // namespace Internal

/**
 * @brief Invokes a function with the given arguments and returns the result.
 */
template <typename TFunc, typename... TArgs>
LUMEN_FORCE_INLINE constexpr auto Invoke ( TFunc &&InFunc, TArgs &&...InArgs )
    -> decltype( static_cast<TFunc &&>( InFunc )( static_cast<TArgs &&>( InArgs )... ) );

/**
 * @brief Invokes a pointer-to-member-data on an object (dereferencing the object if needed).
 */
template <typename TReturnType, typename TObjectType, typename TTargetType>
LUMEN_FORCE_INLINE constexpr auto Invoke ( TReturnType TObjectType::*InMember, TTargetType &&InObject )
    -> decltype( Internal::MaybeDereference( static_cast<TTargetType &&>( InObject ),
                                             std::declval<std::remove_reference_t<TTargetType> *>() ).*
                 InMember );

namespace Internal
{

    template <typename, typename TFunc, typename... TArgs> struct TInvokeResultImpl
    {
        /* */
    };

    template <typename TFunc, typename... TArgs>
    struct TInvokeResultImpl<decltype( static_cast<void>( Invoke( std::declval<TFunc>(), std::declval<TArgs>()... ) ) ),
                             TFunc,
                             TArgs...>
    {
        using Type = decltype( Invoke( std::declval<TFunc>(), std::declval<TArgs>()... ) );
    };

    template <typename, typename TFunc, typename... TArgs> struct TIsInvocableImpl
    {
        static constexpr Bool Value = false;
    };

    template <typename TFunc, typename... TArgs> struct TIsInvocableImpl<TVoidT<TInvokeResultT<TFunc, TArgs...>>, TFunc, TArgs...>
    {
        static constexpr Bool Value = true;
    };

} // namespace Internal

/**
 * @trait TInvokeResult
 * @brief Trait for the type of the result of invoking a function.
 */
template <typename TFunc, typename... TArgs> struct TInvokeResult : Internal::TInvokeResultImpl<void, TFunc, TArgs...>
{
    /* */
};

/**
 * @trait TInvokeResultT
 * @brief Alias for TInvokeResult::Type
 */
template <typename TFunc, typename... TArgs> using TInvokeResultT = typename TInvokeResult<TFunc, TArgs...>::Type;

/**
 * @trait TIsInvocable
 * @brief Whether Invoke(Func, Args...) is well-formed.
 */
template <typename TFunc, typename... TArgs> struct TIsInvocable : Internal::TIsInvocableImpl<void, TFunc, TArgs...>
{
};

/**
 * @trait TIsInvocableV
 * @brief Alias variable for TIsInvocable<...>::Value
 */
template <typename TFunc, typename... TArgs> inline constexpr bool TIsInvocableV = TIsInvocable<TFunc, TArgs...>::Value;

} // namespace Lumen

#include "Inline/Invoke.inl"
