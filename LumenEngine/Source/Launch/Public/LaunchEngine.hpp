/**
 * @file LaunchEngine.hpp
 * @brief Declaration of the entry points of the Engine
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace Launch
{

    Int32 ClientInit ( const Int32 Argc, const AnsiChar *Argv[] );
    Int32 GuardedMain ( const Int32 Argc, const AnsiChar *Argv[] );

} // namespace Launch

} // namespace LumenEngine
