/**
 * @file LoggingCategory.cpp
 * @brief Implementation of the FLogCategory struct, which represents a logging category.
 */

#include "Logging/LoggingCategory.hpp"

LumenEngine::FLogCategory::FLogCategory ( const FString &InCategoryName, const ELogVerbosity::Type InDefaultVerbosity )
    : CategoryName( InCategoryName ), DefaultVerbosity( InDefaultVerbosity )
{
    /* __empty__ */
}
