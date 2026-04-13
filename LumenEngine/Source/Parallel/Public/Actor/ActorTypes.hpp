/**
 * @file ActorTypes.hpp
 * @brief Actor types definition for parallel processing in Lumen Engine.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Logging/LoggingCategory.hpp"

namespace LumenEngine
{

using ActorID = UInt64;

struct FMessage;

namespace Concepts
{

    template <typename Type>
    concept CActor = requires( Type InActor, FMessage InMessage ) {
        { InActor.Receive( InMessage ) } -> std::same_as<void>;
    };

} // namespace Concepts

extern const FLogCategory LogActor;

} // namespace LumenEngine