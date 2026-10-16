/// @file LaunchEngine.hpp
/// Launches the Lumen Engine

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Launch
{

    Int32 GuardedMain ( Int32 InArgc, const AnsiChar *const *InArgv ) noexcept;

}

} // namespace Lumen