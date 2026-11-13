/**
 * @file Invoke.hpp
 * @brief Invokes a callable, a pointer-to-member-data or a pointer-to-member-function.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace Lumen
{

/**
 * @brief Invokes a callable with the given arguments and returns the result.
 */
template <typename TFunc, typename... TArgs>
LUMEN_FORCE_INLINE constexpr auto Invoke ( TFunc &&InFunc, TArgs &&...InArgs )
    -> decltype( std::invoke( std::forward<TFunc>( InFunc ), std::forward<TArgs>( InArgs )... ) );

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

namespace Internal
{

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
 * @trait TIsInvocable
 * @brief Whether Invoke(Func, Args...) is well-formed.
 */
template <typename TFunc, typename... TArgs> struct TIsInvocable : Internal::TIsInvocableImpl<void, TFunc, TArgs...>
{
    /* */
};

/**
 * @trait TIsInvocableV
 * @brief Alias variable for TIsInvocable<...>::Value
 */
template <typename TFunc, typename... TArgs> inline constexpr bool TIsInvocableV = TIsInvocable<TFunc, TArgs...>::Value;

/**
 * @concept CIsInvocable
 * @brief Whether Invoke(Func, Args...) is well-formed.
 */
template <typename TFunc, typename... TArgs>
concept CIsInvocable = TIsInvocableV<TFunc, TArgs...>;

} // namespace Lumen

#include "Inline/Invoke.inl"
