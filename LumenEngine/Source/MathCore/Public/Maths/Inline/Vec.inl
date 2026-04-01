#pragma once

#include "Maths/Vec.hpp"

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

} // namespace Maths

} // namespace LumenEngine
