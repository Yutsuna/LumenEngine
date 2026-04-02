/**
 * @file Matrix.inl
 * @brief Inline implementation of the TMatrix struct.
 */

#pragma once

#include "Maths/Matrix.hpp"

namespace LumenEngine
{

namespace Maths
{

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, Rows, Columns>::TMatrix( const Type &Scalar ) noexcept
    {
        for ( USize ColIndex = 0; ColIndex < Columns; ++ColIndex )
        {
            for ( USize RowIndex = 0; RowIndex < Rows; ++RowIndex )
            {
                if ( ColIndex == RowIndex )
                {
                    Data[ColIndex].Data[RowIndex] = Scalar;
                }
                else
                {
                    Data[ColIndex].Data[RowIndex] = static_cast<Type>( 0 );
                }
            }
        }
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, Rows, Columns>::TMatrix( const ColumnType ( &InColumns )[Columns] ) noexcept
    {
        for ( USize ColIndex = 0; ColIndex < Columns; ++ColIndex )
        {
            Data[ColIndex] = InColumns[ColIndex];
        }
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TVec<Type, Rows> &TMatrix<Type, Rows, Columns>::operator[]( USize ColumnIndex ) noexcept
    {
        return Data[ColumnIndex];
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr const TVec<Type, Rows> &TMatrix<Type, Rows, Columns>::operator[]( USize ColumnIndex ) const noexcept
    {
        return Data[ColumnIndex];
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr Bool TMatrix<Type, Rows, Columns>::operator==( const TMatrix<Type, Rows, Columns> &Other ) const noexcept
    {
        for ( USize ColIndex = 0; ColIndex < Columns; ++ColIndex )
        {
            if ( Data[ColIndex] != Other.Data[ColIndex] )
            {
                return false;
            }
        }
        return true;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr Bool TMatrix<Type, Rows, Columns>::operator!=( const TMatrix<Type, Rows, Columns> &Other ) const noexcept
    {
        return !( *this == Other );
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, Rows, Columns> TMatrix<Type, Rows, Columns>::Identity () noexcept
        requires( Rows == Columns )
    {
        return TMatrix<Type, Rows, Columns>( static_cast<Type>( 1 ) );
    }

    template <typename Type, USize Rows, USize Inner, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &Left, const TMatrix<Type, Inner, Columns> &Right ) noexcept
    {
        TMatrix<Type, Rows, Columns> Result;
        for ( USize ColIndex = 0; ColIndex < Columns; ++ColIndex )
        {
            for ( USize RowIndex = 0; RowIndex < Rows; ++RowIndex )
            {
                Type Sum = static_cast<Type>( 0 );

                for ( USize InnerIndex = 0; InnerIndex < Inner; ++InnerIndex )
                {
                    Sum += Left[InnerIndex].Data[RowIndex] * Right[ColIndex].Data[InnerIndex];
                }
                Result[ColIndex].Data[RowIndex] = Sum;
            }
        }
        return Result;
    }

    template <typename Type, USize Rows, USize Inner, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &Left, const TMatrix<Type, Inner, Columns> &&Right ) noexcept
    {
        return Left * Right;
    }

} // namespace Maths

} // namespace LumenEngine
