/**
 * @file IRHIDebugSink.hpp
 * @brief Declares the logger-independent diagnostic receiver.
 */
#pragma once

#include <Debug/RHIDebugMessage.hpp>

namespace Lumen
{

/**
 * @class IRHIDebugSink
 * @brief Receives synchronously emitted messages; implementations define their threading policy.
 */
class IRHIDebugSink
{
public:

    virtual ~IRHIDebugSink () noexcept                               = default;
    virtual void Emit ( const FRHIDebugMessage &InMessage ) noexcept = 0;
};

} // namespace Lumen
