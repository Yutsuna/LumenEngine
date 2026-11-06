/*
- @file LoggingCategory.cpp
- @brief Implementation of the FLogCategory struct, which represents a logging category.
 */

#include <Logging/LoggingCategory.hpp>

Lumen::FLogCategory::FLogCategory ( const AnsiChar *const InCategoryName, const ELogVerbosity::Type InDefaultVerbosity ) noexcept
    : CategoryName( InCategoryName ), DefaultVerbosity( InDefaultVerbosity )
{
    /** Empty */
}

Lumen::FLogCategory::FLogCategory ( const FStringView InCategoryName, const ELogVerbosity::Type InDefaultVerbosity ) noexcept
    : CategoryName( InCategoryName ), DefaultVerbosity( InDefaultVerbosity )
{
    /** Empty */
}

Lumen::FLogCategory::FLogCategory ( const FString &InCategoryName, const ELogVerbosity::Type InDefaultVerbosity ) noexcept
    : CategoryName( std::move( InCategoryName ) ), DefaultVerbosity( InDefaultVerbosity )
{
    /** Empty */
}
