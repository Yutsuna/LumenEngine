/**
 * @file IsInvocable.hpp
 * @brief Concept whether Invoke() is well-formed.
 */

#pragma once

#include <Templates/Invoke.hpp>

namespace Lumen
{

namespace Concepts
{

    /**
     * @concept CIsInvocable
     * @brief Whether Invoke(Func, Args...) is well-formed.
     */
    template <typename TFunc, typename... TArgs>
    concept CIsInvocable = TIsInvocableV<TFunc, TArgs...>;

} // namespace Concepts

} // namespace Lumen
