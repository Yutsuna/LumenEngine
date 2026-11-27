/**
 * @file Range.hpp
 * @brief Core range access, traits and concepts.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Concepts/SameAs.hpp>

#include <iterator>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Lumen
{

template <class TRangeType> inline constexpr Bool EnableBorrowedRangeV = false;

template <class TElement, USize TExtent> inline constexpr Bool EnableBorrowedRangeV<std::span<TElement, TExtent>> = true;

template <class TCharacter, class TTraits>
inline constexpr Bool EnableBorrowedRangeV<std::basic_string_view<TCharacter, TTraits>> = true;

namespace Algo
{

    namespace Internal
    {

        template <class TRangeType>
        concept CArrayRange = std::is_array_v<std::remove_reference_t<TRangeType>>;

        template <class TRangeType>
        concept CMemberBeginRange = requires( TRangeType &Range ) { Range.begin(); };

        template <class TRangeType>
        concept CMemberEndRange = requires( TRangeType &Range ) { Range.end(); };

        template <class TRangeType>
        concept CAdlBeginRange = not CArrayRange<TRangeType> and not CMemberBeginRange<TRangeType> and
                                 requires( TRangeType &Range ) { begin( Range ); };

        template <class TRangeType>
        concept CAdlEndRange =
            not CArrayRange<TRangeType> and not CMemberEndRange<TRangeType> and requires( TRangeType &Range ) { end( Range ); };

        template <class TIterator>
        concept CAdlIterMove = requires( TIterator &&Iterator ) { iter_move( std::forward<TIterator>( Iterator ) ); };

    } // namespace Internal

    template <class TRangeType>
        requires Internal::CArrayRange<TRangeType>
    [[nodiscard]] constexpr auto Begin ( TRangeType &&InRange ) noexcept
    {
        return InRange + 0;
    }

    template <class TRangeType>
        requires Internal::CMemberBeginRange<TRangeType>
    [[nodiscard]] constexpr decltype( auto ) Begin ( TRangeType &&InRange ) noexcept( noexcept( InRange.begin() ) )
    {
        return InRange.begin();
    }

    template <class TRangeType>
        requires Internal::CAdlBeginRange<TRangeType>
    [[nodiscard]] constexpr decltype( auto ) Begin ( TRangeType &&InRange ) noexcept( noexcept( begin( InRange ) ) )
    {
        return begin( InRange );
    }

    template <class TRangeType>
        requires Internal::CArrayRange<TRangeType>
    [[nodiscard]] constexpr auto End ( TRangeType &&InRange ) noexcept
    {
        return InRange + std::extent_v<std::remove_reference_t<TRangeType>>;
    }

    template <class TRangeType>
        requires Internal::CMemberEndRange<TRangeType>
    [[nodiscard]] constexpr decltype( auto ) End ( TRangeType &&InRange ) noexcept( noexcept( InRange.end() ) )
    {
        return InRange.end();
    }

    template <class TRangeType>
        requires Internal::CAdlEndRange<TRangeType>
    [[nodiscard]] constexpr decltype( auto ) End ( TRangeType &&InRange ) noexcept( noexcept( end( InRange ) ) )
    {
        return end( InRange );
    }

    template <class TIterator>
        requires Internal::CAdlIterMove<TIterator>
    [[nodiscard]] constexpr decltype( auto )
    IterMove ( TIterator &&InIterator ) noexcept( noexcept( iter_move( std::forward<TIterator>( InIterator ) ) ) )
    {
        return iter_move( std::forward<TIterator>( InIterator ) );
    }

    template <class TIterator>
        requires( not Internal::CAdlIterMove<TIterator> )
    [[nodiscard]] constexpr decltype( auto )
    IterMove ( TIterator &&InIterator ) noexcept( noexcept( *std::forward<TIterator>( InIterator ) ) )
    {
        if constexpr ( std::is_lvalue_reference_v<decltype( *std::forward<TIterator>( InIterator ) )> )
        {
            return std::move( *std::forward<TIterator>( InIterator ) );
        }
        else
        {
            return *std::forward<TIterator>( InIterator );
        }
    }

    namespace Internal
    {

        template <class TRangeType> using TRangeIteratorT    = decltype( Algo::Begin( std::declval<TRangeType>() ) );
        template <class TRangeType> using TRangeSentinelT    = decltype( Algo::End( std::declval<TRangeType>() ) );
        template <class TRangeType> using TRangeReferenceT   = decltype( *Algo::Begin( std::declval<TRangeType>() ) );
        template <class TRangeType> using TRangeElementValue = std::iter_value_t<TRangeIteratorT<TRangeType>>;
        template <class TRangeType> using TRangeElementPtr   = std::remove_reference_t<TRangeReferenceT<TRangeType>> *;

    } // namespace Internal

} // namespace Algo

namespace Concepts
{

    template <class TRangeType>
    concept CRange = requires( TRangeType &&Range ) {
        Algo::Begin( std::forward<TRangeType>( Range ) );
        Algo::End( std::forward<TRangeType>( Range ) );
    };

    template <class TRangeType>
    concept CCommonRange =
        CRange<TRangeType> and CSameAs<Algo::Internal::TRangeIteratorT<TRangeType>, Algo::Internal::TRangeSentinelT<TRangeType>>;

    template <class TRangeType>
    concept CInputRange =
        CRange<TRangeType> and std::input_iterator<Algo::Internal::TRangeIteratorT<TRangeType>> and
        requires( Algo::Internal::TRangeIteratorT<TRangeType> First, Algo::Internal::TRangeSentinelT<TRangeType> Last ) {
            static_cast<Bool>( First != Last );
            ++First;
        };

    template <class TRangeType>
    concept CRandomAccessRange = CInputRange<TRangeType> and CCommonRange<TRangeType> and
                                 std::random_access_iterator<Algo::Internal::TRangeIteratorT<TRangeType>>;

    template <class TRangeType>
    concept CBorrowedRange = CRange<TRangeType> and
                             ( std::is_lvalue_reference_v<TRangeType> or EnableBorrowedRangeV<std::remove_cvref_t<TRangeType>> );

} // namespace Concepts

namespace Algo
{

    namespace Internal
    {

        template <class TRangeType>
        concept CPointerReturnRange = Concepts::CBorrowedRange<TRangeType> and std::is_reference_v<TRangeReferenceT<TRangeType>>;

        template <class TRangeType> inline constexpr Bool bConsumeRangeElements = not Concepts::CBorrowedRange<TRangeType>;

    } // namespace Internal

} // namespace Algo

} // namespace Lumen
