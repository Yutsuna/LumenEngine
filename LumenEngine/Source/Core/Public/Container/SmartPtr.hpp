/**
 * @file SmartPtr.hpp
 * @brief Declaration of smart pointer classes for memory management.
 */

#pragma once

// TODO: own implementation of smart pointers
#include <SDL3/SDL_gamepad.h>
#include <memory>

namespace LumenEngine
{

// TODO: own implementation of TSharedPtr

template <typename Type>
class TSharedPtr : public std::shared_ptr<Type>
{
public:

private:
};

} // namespace LumenEngine
