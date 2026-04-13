#pragma once

#include "Maths/Vec.hpp"

#include <cmath>

namespace LumenEngine
{

namespace Maths
{

    /**
     * Construct a Vec by converting from another Vec of a different type but the same dimension
     *
     * @param Other The Vec to convert from
     * @return A new Vec with component values converted from Other
     */

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>

    template <typename OtherType>
    constexpr TVec<Type, Dimension>::TVec( const TVec<OtherType, Dimension> &Other ) noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] = static_cast<Type>( Other.Data[Index] );
        }
    }

    /**
     * Construct a Vec with all components set to the same value
     *
     * @param Value The value to set all components to
     * @return A vector with all components set to Value
     */

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>

    constexpr TVec<Type, Dimension>::TVec( const Type &Value ) noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] = Value;
        }
    }

    /**
     * Construct a Vec with the given component values
     *
     * @param Args The values to set each component to
     * @return A vector with components set to the given values
     */
    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>

    template <typename... Arguments>
        requires( sizeof...( Arguments ) == Dimension && ( std::is_convertible_v<Arguments, Type> && ... ) )

    constexpr TVec<Type, Dimension>::TVec( Arguments &&...Args ) noexcept : Private::TVecData<Type, Dimension>{ { static_cast<Type>( Args )... } }
    {
        /* Empty */
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>

    constexpr Bool TVec<Type, Dimension>::operator==( const TVec<Type, Dimension> &Other ) const noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            if ( this->Data[Index] != Other.Data[Index] )
            {
                return false;
            }
        }
        return true;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>

    constexpr Bool TVec<Type, Dimension>::operator!=( const TVec<Type, Dimension> &Other ) const noexcept
    {
        return not( *this == Other );
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator+( const TVec &Other ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] + Other.Data[Index];
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator-( const TVec &Other ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] - Other.Data[Index];
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator*( const TVec &Other ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] * Other.Data[Index];
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator/( const TVec &Other ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] / Other.Data[Index];
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator+( Type Scalar ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] + Scalar;
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator-( Type Scalar ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] - Scalar;
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator*( Type Scalar ) const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] * Scalar;
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator/( Type Scalar ) const noexcept
    {
        [[assume( Scalar != static_cast<Type>( 0 ) )]];
        TVec Result;
        const Type InvScalar = static_cast<Type>( 1 ) / Scalar;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = this->Data[Index] * InvScalar;
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> &TVec<Type, Dimension>::operator+=( const TVec &Other ) noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] += Other.Data[Index];
        }
        return *this;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> &TVec<Type, Dimension>::operator-=( const TVec &Other ) noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] -= Other.Data[Index];
        }
        return *this;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> &TVec<Type, Dimension>::operator*=( const TVec &Other ) noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] *= Other.Data[Index];
        }
        return *this;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> &TVec<Type, Dimension>::operator/=( const TVec &Other ) noexcept
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] /= Other.Data[Index];
        }
        return *this;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::operator-() const noexcept
    {
        TVec Result;
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result.Data[Index] = -this->Data[Index];
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr Type TVec<Type, Dimension>::Dot ( const TVec &Other ) const noexcept
    {
        Type Result = static_cast<Type>( 0 );
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            Result += this->Data[Index] * Other.Data[Index];
        }
        return Result;
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::Cross ( const TVec &Other ) const noexcept
        requires( Dimension == 3 )
    {
        return TVec( ( this->Y * Other.Z ) - ( this->Z * Other.Y ), ( this->Z * Other.X ) - ( this->X * Other.Z ), ( this->X * Other.Y ) - ( this->Y * Other.X ) );
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr Type TVec<Type, Dimension>::LengthSquared () const noexcept
    {
        return this->Dot( *this );
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr Type TVec<Type, Dimension>::Length () const noexcept
    {
        return std::sqrt( this->LengthSquared() );
    }

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>
    constexpr TVec<Type, Dimension> TVec<Type, Dimension>::Normalize () const noexcept
    {
        const Type Len = this->Length();
        if ( Len > static_cast<Type>( 0 ) )
        {
            return *this / Len;
        }
        return *this;
    }

} // namespace Maths

} // namespace LumenEngine
