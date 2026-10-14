/// @file NonCopyable.hpp
/// @brief Declaration of NonCopyable class for preventing copy construction and assignment.

#pragma once

#include "CoreExport.hpp"

namespace Lumen
{

///
/// @class FNonCopyable
/// @brief Prevents copy construction and assignment.
///
class CORE_EXPORT FNonCopyable
{
public:

    FNonCopyable ( const FNonCopyable & ) noexcept           = delete;
    FNonCopyable &operator=( const FNonCopyable & ) noexcept = delete;

protected:

    FNonCopyable () noexcept  = default;
    ~FNonCopyable () noexcept = default;
};

} // namespace Lumen