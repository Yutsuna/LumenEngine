/// @file Overloaded.hpp
/// @brief Overload-set builder for std::visit.

#pragma once

namespace Lumen
{

namespace Meta
{

    /// @brief Combines several callables into one overload set, for std::visit.
    /// @details Per-alternative lambdas + a generic fallback: `std::visit( FOverloaded{...}, Var )`.
    template <typename... Fs> struct FOverloaded : Fs...
    {
        using Fs::operator()...;
    };

    template <typename... Fs> FOverloaded( Fs... ) -> FOverloaded<Fs...>;

} // namespace Meta

} // namespace Lumen
