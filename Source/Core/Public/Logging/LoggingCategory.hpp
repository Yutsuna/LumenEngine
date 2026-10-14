///
/// @file LoggingCategory.hpp
/// @brief Definition of the FLogCategory struct, which represents a logging category.
///

#pragma once

#include <CoreExport.hpp>

#include <Logging/LoggingVerbosity.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

namespace Lumen
{

struct CORE_EXPORT FLogCategory
{

    FString CategoryName;
    ELogVerbosity::Type DefaultVerbosity;

    FLogCategory ( const FString &InCategoryName, ELogVerbosity::Type InDefaultVerbosity = ELogVerbosity::Info ) noexcept;
    FLogCategory ( FStringView InCategoryName, ELogVerbosity::Type InDefaultVerbosity = ELogVerbosity::Info ) noexcept;
};

} // namespace Lumen