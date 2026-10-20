/// @file LoggingCategory.cpp
/// @brief Implementation of the FLogCategory struct, which represents a logging category.

#include <Logging/LoggingCategory.hpp>

Lumen::FLogCategory::FLogCategory ( const FString &InCategoryName, const ELogVerbosity::Type InDefaultVerbosity ) noexcept
    : DefaultVerbosity( InDefaultVerbosity )
{
    try
    {
        CategoryName = InCategoryName;
    }
    catch ( ... )
    {
        CategoryName = FString();
    }
}

Lumen::FLogCategory::FLogCategory ( const FStringView InCategoryName, const ELogVerbosity::Type InDefaultVerbosity ) noexcept
    : DefaultVerbosity( InDefaultVerbosity )

{
    try
    {
        CategoryName = InCategoryName;
    }
    catch ( ... )
    {
        CategoryName = FString();
    }
}