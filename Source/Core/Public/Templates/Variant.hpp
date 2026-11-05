/// @file Variant.hpp
/// @brief Defines the Variant template.

#pragma once

#include <Core/CoreTypes.hpp>

#include <variant>

namespace Lumen
{

template <class... Types> using TVariant                                                    = std::variant<Types...>;
template <typename Type> inline constexpr Bool IsVariantV                                   = false;
template <typename... TArguments> inline constexpr Bool IsVariantV<TVariant<TArguments...>> = true;

} // namespace Lumen
