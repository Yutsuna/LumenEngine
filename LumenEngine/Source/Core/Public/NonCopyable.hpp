/**
 * @file NonCopyable.hpp
 * @brief Base class to prevent copying and assignment of derived classes
 */

#pragma once

namespace LumenEngine
{

class FNonCopyable
{

public:

    FNonCopyable ( const FNonCopyable & )           = delete;
    FNonCopyable &operator=( const FNonCopyable & ) = delete;

protected:

    FNonCopyable ()  = default;
    ~FNonCopyable () = default;
};

} // namespace LumenEngine
