/**
 * @file Function.hpp
 * @brief TFunction type definition.
 */

#pragma once

#include <functional>

namespace Lumen
{

template <typename TFunc> using TFunction = std::function<TFunc>;

} // namespace Lumen
