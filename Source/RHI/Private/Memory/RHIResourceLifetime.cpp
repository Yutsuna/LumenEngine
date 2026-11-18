/**
 * @file RHIResourceLifetime.cpp
 * @brief Implements lifetime overlap checks.
 */
#include <Memory/RHIResourceLifetime.hpp>

Lumen::Bool Lumen::FRHIResourceLifetime::Overlaps ( const FRHIResourceLifetime &InOther ) const noexcept
{
    return IsValid() and InOther.IsValid() and Begin < InOther.End and InOther.Begin < End;
}
