/**
 * @file Span.hpp
 * @brief FSpan class definition for non-owning views of contiguous data in Lumen Engine.
 */

#pragma once

#include <span>

namespace LumenEngine
{

template <typename ElementType> using TSpan = std::span<ElementType>;

}
