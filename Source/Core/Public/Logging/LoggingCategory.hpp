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

    const AnsiChar *const CategoryName;
    ELogVerbosity::Type DefaultVerbosity;

    FLogCategory ( const AnsiChar * InCategoryName, ELogVerbosity::Type InDefaultVerbosity = ELogVerbosity::Info ) noexcept;
};

} // namespace Lumen