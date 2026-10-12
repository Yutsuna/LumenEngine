/// @file Vector.hpp
/// @brief Definition of the TVector class

#pragma once

#include <Core/CoreTypes.hpp>
#include <vector>

namespace Lumen
{

/// TODO: Implement custom TVector class
template <typename Type, typename Allocator = std::allocator<Type>> using TVector = std::vector<Type, Allocator>;

} // namespace Lumen