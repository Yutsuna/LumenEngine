/// @file Matrix.hpp
/// @brief Lumen column-major matrix

#pragma once

#include <Core/CoreTypes.hpp>

#include <Maths/Vec.hpp>

namespace Lumen
{

namespace Maths
{

    namespace Concepts
    {

        template <USize Rows, USize Columns>
        concept CMatrixDimension = Rows > 0 and Rows <= 4 and Columns > 0 and Columns <= 4;

        template <USize Rows, USize Columns>
        concept CSquareMatrix = CMatrixDimension<Rows, Columns> and ( Rows == Columns );

    } // namespace Concepts

    ///
    /// @struct TMatrix
    /// @brief A column-major matrix template
    /// @tparam Type The scalar type (Float32, Float64...)
    /// @tparam Rows Number of rows
    /// @tparam Columns Number of columns
    ///
    template <typename Type, USize Rows, USize Columns>
        requires Concepts::CMatrixDimension<Rows, Columns>
    struct TMatrix final
    {
    public:

        using ColumnType = TVec<Type, Rows>;
        using RowType    = TVec<Type, Columns>;

    private:

        ColumnType Data[Columns];

    public:

        constexpr TMatrix () noexcept = default;
    };

} // namespace Maths

} // namespace Lumen
