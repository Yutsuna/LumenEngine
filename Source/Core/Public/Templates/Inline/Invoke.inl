/**
 * @file Invoke.inl
 * @brief Inline implementation of the Invoke template function.
 */

#pragma once

#include <Templates/Invoke.hpp>

/**
 * Public
 */

template <typename TFunc, typename... TArgs>
constexpr auto Lumen::Invoke ( TFunc &&InFunc, TArgs &&...InArgs ) noexcept( std::is_nothrow_invocable_v<TFunc, TArgs...> )
    -> decltype( std::invoke( std::forward<TFunc>( InFunc ), std::forward<TArgs>( InArgs )... ) )
{
    return std::invoke( std::forward<TFunc>( InFunc ), std::forward<TArgs>( InArgs )... );
}
