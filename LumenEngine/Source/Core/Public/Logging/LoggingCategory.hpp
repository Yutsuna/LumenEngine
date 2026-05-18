/**
 * @file LoggingCategory.hpp
 * @brief Definition of the FLogCategory struct, which represents a logging category.
 */

#pragma once

#include "Container/String.hpp"
#include "Definitions.hpp"
#include "Logging/LoggingVerbosity.hpp"

namespace LumenEngine
{

struct LUMEN_ENGINE_API FLogCategory
{

    FString CategoryName;
    ELogVerbosity::Type DefaultVerbosity;

    FLogCategory ( FString &InCategoryName, const ELogVerbosity::Type InDefaultVerbosity = ELogVerbosity::Info );
    FLogCategory ( FStringView InCategoryName, const ELogVerbosity::Type InDefaultVerbosity = ELogVerbosity::Info );
};

} // namespace LumenEngine
