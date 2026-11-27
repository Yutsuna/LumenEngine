/**
 * @file Application.cpp
 * @brief Implementation of the FLuminaApplication class.
 */

#include <Application.hpp>

#include <Logging/Logger.hpp>

/**
 * Registration
 */

LUMEN_REGISTER_MODULE( Lumen::FLuminaApplication )

/**
 * Private
 */

namespace
{

LUMEN_LOG_DEFINE_CATEGORY( LogLumina, "Lumina" );

} // namespace

/**
 * Public
 */

Lumen::FLuminaApplication::FLuminaApplication () noexcept
{
    LUMEN_LOG_VERY_VERBOSE( LogLumina, "FLuminaApplication constructed" );
}
