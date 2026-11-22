/**
 * @file SameAs.hpp
 * @brief Concept to check if two types are exactly the same.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/IsBool.hpp>

namespace Lumen
{

namespace Internal
{

    template <typename TObject, typename TCompare> struct TIsSame : TFalseType
    {
        /* */
    };

    template <typename TObject> struct TIsSame<TObject, TObject> : TTrueType
    {
        /* */
    };

    template <typename TObject, typename TCompare> inline constexpr Bool TIsSameV = TIsSame<TObject, TCompare>::Value;

} // namespace Internal

namespace Concepts
{

    /**
     * @concept CSameAs
     * @brief Checks if two types are exactly the same.
     */
    template <typename TObject, typename TCompare>
    concept CSameAs = Internal::TIsSameV<TObject, TCompare>;

    /**
     * @concept CIsSame
     * @brief Checks if two types are exactly the same.
     */
    template <typename TObject, typename TCompare>
    concept CIsSame = CSameAs<TObject, TCompare>;

} // namespace Concepts

} // namespace Lumen
