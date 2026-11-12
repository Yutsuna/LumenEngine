/**
 * @file IsInvocable.hpp
 * @brief Concept whether Invoke() is well-formed.
 */

#pragma once

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
