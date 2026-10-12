///
/// @file Array.hpp
/// @brief Declaration of the Array container class.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <array>

namespace Lumen
{
// TODO: Implement custom array class
template <typename Type, USize Size> using TArray = std::array<Type, Size>;
} // namespace Lumen
