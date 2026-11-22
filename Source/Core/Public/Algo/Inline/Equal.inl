/**
 * @file Equal.inl
 * @brief Inline implementation of Equal algorithms.
 */

#pragma once

#include <Algo/Equal.hpp>

#include <utility>

/**
 * Public
 */

template <typename TRangeA, typename TRangeB, typename TPredicate>
    requires( Lumen::Concepts::CConvertibleTo<Lumen::TInvokeResultT<TPredicate,
                                                                    Lumen::Algo::Internal::TRangeReferenceT<TRangeA>,
                                                                    Lumen::Algo::Internal::TRangeReferenceT<TRangeB>>,
                                              Lumen::Bool> )
constexpr Lumen::Bool Lumen::Algo::Equal ( TRangeA &&InRangeA, TRangeB &&InRangeB, TPredicate &&InPredicate )
{
    auto &&Pred = std::forward<TPredicate>( InPredicate );
    auto ItA    = std::begin( InRangeA );
    auto ItB    = std::begin( InRangeB );
    auto EndA   = std::end( InRangeA );
    auto EndB   = std::end( InRangeB );

    for ( ; ItA != EndA and ItB != EndB; ++ItA, ++ItB )
    {
        if ( not Lumen::Invoke( Pred, *ItA, *ItB ) )
        {
            return false;
        }
    }

    return ItA == EndA and ItB == EndB;
}

template <typename TRangeA, typename TRangeB> constexpr Lumen::Bool Lumen::Algo::Equal ( TRangeA &&InRangeA, TRangeB &&InRangeB )
{
    return Equal( std::forward<TRangeA>( InRangeA ), std::forward<TRangeB>( InRangeB ),
                  [] ( auto &&A, auto &&B ) noexcept -> Lumen::Bool
    {
        return A == B;
    } );
}
