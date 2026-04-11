/**
 * @file Matrix.inl
 * @brief Inline implementation of the TMatrix struct.
 */

#pragma once

#include "Maths/Matrix.hpp"

#include <cmath>

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

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::Translate ( const TVec<Type, 3> &InTranslation ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        TMatrix<Type, 4, 4> Result = TMatrix<Type, 4, 4>::Identity();

        Result[3][0] = InTranslation.X;
        Result[3][1] = InTranslation.Y;
        Result[3][2] = InTranslation.Z;

        return Result;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::Scale ( const TVec<Type, 3> &InScale ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        TMatrix<Type, 4, 4> Result( static_cast<Type>( 0 ) );

        Result[0][0] = InScale.X;
        Result[1][1] = InScale.Y;
        Result[2][2] = InScale.Z;
        Result[3][3] = static_cast<Type>( 1 );

        return Result;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::RotateX ( Type InAngleRadians ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        const Type CosAngle = std::cos( InAngleRadians );
        const Type SinAngle = std::sin( InAngleRadians );

        TMatrix<Type, 4, 4> Result = TMatrix<Type, 4, 4>::Identity();

        Result[1][1] = CosAngle;
        Result[1][2] = SinAngle;
        Result[2][1] = -SinAngle;
        Result[2][2] = CosAngle;

        return Result;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::RotateY ( Type InAngleRadians ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        const Type CosAngle = std::cos( InAngleRadians );
        const Type SinAngle = std::sin( InAngleRadians );

        TMatrix<Type, 4, 4> Result = TMatrix<Type, 4, 4>::Identity();

        Result[0][0] = CosAngle;
        Result[0][2] = -SinAngle;
        Result[2][0] = SinAngle;
        Result[2][2] = CosAngle;

        return Result;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::RotateZ ( Type InAngleRadians ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        const Type CosAngle = std::cos( InAngleRadians );
        const Type SinAngle = std::sin( InAngleRadians );

        TMatrix<Type, 4, 4> Result = TMatrix<Type, 4, 4>::Identity();

        Result[0][0] = CosAngle;
        Result[0][1] = SinAngle;
        Result[1][0] = -SinAngle;
        Result[1][1] = CosAngle;

        return Result;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::Perspective ( Type InFieldOfViewRadians, Type InAspectRatio, Type InNearPlane, Type InFarPlane ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        const Type TanHalfFov = std::tan( InFieldOfViewRadians / static_cast<Type>( 2 ) );

        TMatrix<Type, 4, 4> Result( static_cast<Type>( 0 ) );

        Result[0][0] = static_cast<Type>( 1 ) / ( InAspectRatio * TanHalfFov );
        Result[1][1] = static_cast<Type>( -1 ) / TanHalfFov;
        Result[2][2] = InFarPlane / ( InFarPlane - InNearPlane );
        Result[2][3] = static_cast<Type>( 1 );
        Result[3][2] = -( InFarPlane * InNearPlane ) / ( InFarPlane - InNearPlane );

        return Result;
    }

    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    constexpr TMatrix<Type, 4, 4> TMatrix<Type, Rows, Columns>::LookAt ( const TVec<Type, 3> &InEye, const TVec<Type, 3> &InTarget, const TVec<Type, 3> &InUp ) noexcept
        requires( Rows == 4 && Columns == 4 )
    {
        const TVec<Type, 3> F = ( InTarget - InEye ).Normalize();
        const TVec<Type, 3> R = InUp.Cross( F ).Normalize();
        const TVec<Type, 3> U = F.Cross( R );

        TMatrix<Type, 4, 4> Result = TMatrix<Type, 4, 4>::Identity();

        Result[0][0] = R.X;
        Result[1][0] = R.Y;
        Result[2][0] = R.Z;

        Result[0][1] = U.X;
        Result[1][1] = U.Y;
        Result[2][1] = U.Z;

        Result[0][2] = F.X;
        Result[1][2] = F.Y;
        Result[2][2] = F.Z;

        Result[3][0] = -R.Dot( InEye );
        Result[3][1] = -U.Dot( InEye );
        Result[3][2] = -F.Dot( InEye );

        return Result;
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
