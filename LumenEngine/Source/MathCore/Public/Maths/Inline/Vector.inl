#pragma once

#include "Maths/Vector.hpp"

namespace LumenEngine
{

namespace Math
{

    /**
     * Construct a Vector with all components set to the same value
     *
     * @param Value The value to set all components to
     * @return A vector with all components set to Value
     */

    template <typename Type, USize Dimension>
        requires CVectorDimension<Type, Dimension>

    constexpr TVector<Type, Dimension>::TVector( const Type &Value )
    {
        for ( USize Index = 0; Index < Dimension; ++Index )
        {
            this->Data[Index] = Value;
        }
    }

    /**
     * Construct a Vector with the given component values
     *
     * @param Args The values to set each component to
     * @return A vector with components set to the given values
     */
    template <typename Type, USize Dimension>
        requires CVectorDimension<Type, Dimension>

    template <typename... Arguments>
        requires( sizeof...( Arguments ) == Dimension && ( std::is_convertible_v<Arguments, Type> && ... ) )

    constexpr TVector<Type, Dimension>::TVector( Arguments &&...Args ) : Private::TVectorData<Type, Dimension>{ { static_cast<Type>( Args )... } }
    {
        /* Empty */
    }

} // namespace Math

} // namespace LumenEngine
