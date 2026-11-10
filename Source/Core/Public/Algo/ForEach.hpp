/**
 * @file ForEach.hpp
 * @brief Applies a function to each element in a range.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Concepts/ConvertibleTo.hpp>
#include <Templates/Invoke.hpp>

namespace Lumen
{

namespace Algo
{

    /**
     * @brief Applies a function to each element in a range that satisfies a predicate.
     *
     * @param Object The range to iterate over.
     * @param Predicate The predicate to satisfy.
     * @param Func The function to apply to each element.
     */
    template <typename TObject, typename TPredicate, typename TFunc>
        requires( Concepts::CConvertibleTo<TInvokeResultT<TFunc, TObject &>, Bool> )
    LUMEN_FORCE_INLINE void ForEach ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func );

    /**
     * @brief Applies a function to each element in a range.
     *
     * @param Object The range to iterate over.
     * @param Func The function to apply to each element.
     */
    template <typename TObject, typename TFunc> LUMEN_FORCE_INLINE void ForEach ( TObject &&Object, TFunc &&Func );

    /**
     * @brief Applies a function to each element until the function returns true.
     *
     * @param Object The range to iterate over.
     * @param Func The function to apply to each element.
     * @return True if the loop was stopped early, false if the range was exhausted.
     */
    template <typename TObject, typename TFunc>
        requires( Concepts::CConvertibleTo<TInvokeResultT<TFunc, TObject &>, Bool> )
    [[nodiscard]] LUMEN_FORCE_INLINE Bool ForEachUntil ( TObject &&Object, TFunc &&Func );

    /**
     * @brief Applies a function to each element satisfying a predicate until the function returns true.
     *
     * @param Object The range to iterate over.
     * @param Predicate The predicate to apply to each element.
     * @param Func The function to apply to each element.
     * @return True if the loop was stopped early, false if the range was exhausted.
     */
    template <typename TObject, typename TPredicate, typename TFunc>
        requires( Concepts::CConvertibleTo<TInvokeResultT<TFunc, TObject &>, Bool> )
    [[nodiscard]] LUMEN_FORCE_INLINE Bool ForEachUntil ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func );

} // namespace Algo

} // namespace Lumen

#include "Inline/ForEach.inl"
