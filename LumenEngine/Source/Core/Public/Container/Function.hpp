/**
 * @file Function.hpp
 * @brief Declaration of the Function wrapper class for type-erased callable objects.
 */

#pragma once

#include <functional>

namespace LumenEngine
{

template <typename FunctionSignature> using TFunction = std::function<FunctionSignature>;

}
