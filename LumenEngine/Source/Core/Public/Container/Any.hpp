/**
 * @file Any.hpp
 * @brief FAny class definition for type-erased storage in Lumen Engine.
 */

#pragma once

#include <any>

namespace LumenEngine
{

using FAny = std::any;

template <typename Type> [[nodiscard]] inline Type TAnyCast ( const FAny &InAny )
{
    return std::any_cast<Type>( InAny );
}

} // namespace LumenEngine