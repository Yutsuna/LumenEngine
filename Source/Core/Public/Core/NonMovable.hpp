///
/// @file NonMovable.hpp
/// @brief Base class to prevent moving of derived classes
///

#pragma once

namespace Lumen
{

///
/// @class FNonMovable
/// @brief Base class to prevent moving of derived classes
///
class FNonMovable
{

public:

    FNonMovable ( FNonMovable && )           = delete;
    FNonMovable &operator=( FNonMovable && ) = delete;

protected:

    FNonMovable ()  = default;
    ~FNonMovable () = default;
};

} // namespace Lumen