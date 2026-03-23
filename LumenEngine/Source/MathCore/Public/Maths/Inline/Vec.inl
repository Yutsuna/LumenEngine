#pragma once

#include "Maths/Vec.hpp"

namespace LumenEngine
{

namespace Maths
{

    /**
     * Construct a Vec with all components set to the same value
     *
     * @param Value The value to set all components to
     * @return A vector with all components set to Value
     */

    template <typename Type, USize Dimension>
        requires CVecDimension<Type, Dimension>

    constexpr TVec<Type, Dimension>::TVec( const Type &Value )
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

    constexpr TVec<Type, Dimension>::TVec( Arguments &&...Args ) : Private::TVecData<Type, Dimension>{ { static_cast<Type>( Args )... } }
    {
        /* Empty */
    }

} // namespace Maths

} // namespace LumenEngine
