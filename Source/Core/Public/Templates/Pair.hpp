/**
 * @file Pair.hpp
 * @brief Pair template utilities.
 */

#pragma once

#include <utility>

namespace Lumen
{

/**
 * @class TPair
 * @tparam TFirstType The type of the first element.
 * @tparam TSecType The type of the second element.
 */
template <class TFirstType, class TSecType> using TPair = std::pair<TFirstType, TSecType>;

} // namespace Lumen
