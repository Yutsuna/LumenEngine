/**
 * @file ForEach.hpp
 * @brief Applies a function to each element in a range.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Algo/RangeTraits.hpp>

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
     * @param Func The function to apply to each element that satisfies Predicate.
     */
    template <typename TObject, typename TPredicate, typename TFunc>
        requires( Concepts::CConvertibleTo<TInvokeResultT<TPredicate, Internal::TRangeReferenceT<TObject>>, Bool> )
    LUMEN_FORCE_INLINE constexpr void ForEach ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func );

    /**
     * @brief Applies a function to each element in a range.
     *
     * @param Object The range to iterate over.
     * @param Func The function to apply to each element.
     */
    template <typename TObject, typename TFunc> LUMEN_FORCE_INLINE constexpr void ForEach ( TObject &&Object, TFunc &&Func );

    /**
     * @brief Iterates until Func returns true and returns the matching element.
     *
     * @param Object The range to iterate over.
     * @param Func The predicate selecting the element to stop on.
     * @return Pointer to the first element for which Func returned true,
     *         or nullptr if the range was exhausted.
     */
    template <typename TObject, typename TFunc>
        requires( Concepts::CConvertibleTo<TInvokeResultT<TFunc, Internal::TRangeReferenceT<TObject>>, Bool> )
    LUMEN_FORCE_INLINE constexpr Internal::TRangeElementPtr<TObject> ForEachUntil ( TObject &&Object, TFunc &&Func );

    /**
     * @brief Applies Func to each element satisfying Predicate, stopping early.
     *
     * @param Object The range to iterate over.
     * @param Predicate The predicate to apply to each element.
     * @param Func The function to apply to each element satisfying Predicate.
     * @return True if the loop was stopped early, false if the range was exhausted.
     */
    template <typename TObject, typename TPredicate, typename TFunc>
        requires( Concepts::CConvertibleTo<TInvokeResultT<TPredicate, Internal::TRangeReferenceT<TObject>>, Bool> &&
                  Concepts::CConvertibleTo<TInvokeResultT<TFunc, Internal::TRangeReferenceT<TObject>>, Bool> )
    LUMEN_FORCE_INLINE constexpr Bool ForEachUntil ( TObject &&Object, TPredicate &&Predicate, TFunc &&Func );

} // namespace Algo

} // namespace Lumen

#include "Inline/ForEach.inl"
