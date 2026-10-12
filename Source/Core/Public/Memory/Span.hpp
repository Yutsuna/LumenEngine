/// @file Span.hpp
/// @brief Span is a non-owning view of a contiguous sequence of objects.

#pragma once

#include <Core/CoreTypes.hpp>

#include <span>

namespace Lumen
{

template <typename ObjectType, USize ObjectSize = std::dynamic_extent> using TSpan = std::span<ObjectType, ObjectSize>;

}