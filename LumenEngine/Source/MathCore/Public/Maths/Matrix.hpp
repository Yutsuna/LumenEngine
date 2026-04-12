/**
 * @file Matrix.hpp
 * @brief Declaration of the TMatrix struct for mathematical matrix operations.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "Maths/Vec.hpp"

namespace LumenEngine
{

namespace Maths
{

    template <USize Rows, USize Columns>
    concept CMatrixDimension = Rows > 0 && Rows <= 4 && Columns > 0 && Columns <= 4;

    /**
     * @struct TMatrix
     * @brief A column-major matrix template.
     * @tparam Type The scalar type (Float32, Float64, etc.)
     * @tparam Rows Number of rows
     * @tparam Columns Number of columns
     */
    template <typename Type, USize Rows, USize Columns>
        requires CMatrixDimension<Rows, Columns>
    struct LUMEN_ENGINE_API TMatrix final
    {
    public:

        using ColumnType = TVec<Type, Rows>;
        using RowType    = TVec<Type, Columns>;

    private:

        ColumnType Data[Columns];

    public:

        constexpr TMatrix () noexcept = default;

        /** @brief Construct a matrix with a scalar value along the diagonal (Identity * Scalar) */
        constexpr explicit TMatrix ( const Type &Scalar ) noexcept;

        /** @brief Construct a matrix from an array of columns */
        constexpr explicit TMatrix ( const ColumnType ( &InColumns )[Columns] ) noexcept;

        /** @brief Access a column by index */
        constexpr ColumnType &operator[]( USize ColumnIndex ) noexcept;
        /** @brief Access a column by index */
        constexpr const ColumnType &operator[]( USize ColumnIndex ) const noexcept;

        constexpr Bool operator==( const TMatrix<Type, Rows, Columns> &Other ) const noexcept;
        constexpr Bool operator!=( const TMatrix<Type, Rows, Columns> &Other ) const noexcept;

        /** @brief Returns an identity matrix. Only valid for square matrices. */
        static constexpr TMatrix<Type, Rows, Columns> Identity () noexcept
            requires( Rows == Columns );

        /** @brief Create a translation matrix */
        static constexpr TMatrix<Type, 4, 4> Translate ( const TVec<Type, 3> &InTranslation ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create a scaling matrix */
        static constexpr TMatrix<Type, 4, 4> Scale ( const TVec<Type, 3> &InScale ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create a rotation matrix around X axis (radians) */
        static constexpr TMatrix<Type, 4, 4> RotateX ( Type InAngleRadians ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create a rotation matrix around Y axis (radians) */
        static constexpr TMatrix<Type, 4, 4> RotateY ( Type InAngleRadians ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create a rotation matrix around Z axis (radians) */
        static constexpr TMatrix<Type, 4, 4> RotateZ ( Type InAngleRadians ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create a perspective projection matrix */
        static constexpr TMatrix<Type, 4, 4> Perspective ( Type InFieldOfViewRadians, Type InAspectRatio, Type InNearPlane, Type InFarPlane ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create an orthographic projection matrix (Vulkan clip space) */
        static constexpr TMatrix<Type, 4, 4> Orthographic ( Type InWidth, Type InHeight, Type InNearPlane, Type InFarPlane ) noexcept
            requires( Rows == 4 && Columns == 4 );

        /** @brief Create a look-at view matrix */
        static constexpr TMatrix<Type, 4, 4> LookAt ( const TVec<Type, 3> &InEye, const TVec<Type, 3> &InTarget, const TVec<Type, 3> &InUp ) noexcept
            requires( Rows == 4 && Columns == 4 );
    };

    using FMatrix2x2f = TMatrix<Float32, 2, 2>;
    using FMatrix3x3f = TMatrix<Float32, 3, 3>;
    using FMatrix4x4f = TMatrix<Float32, 4, 4>;
    using FMatrix3x4f = TMatrix<Float32, 3, 4>;
    using FMatrix4x3f = TMatrix<Float32, 4, 3>;

    using FMatrix2x2d = TMatrix<Float64, 2, 2>;
    using FMatrix3x3d = TMatrix<Float64, 3, 3>;
    using FMatrix4x4d = TMatrix<Float64, 4, 4>;
    using FMatrix3x4d = TMatrix<Float64, 3, 4>;
    using FMatrix4x3d = TMatrix<Float64, 4, 3>;

    /** @brief Matrix Multiplication: (R x N) * (N x C) = (R x C) */
    template <typename Type, USize Rows, USize Inner, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &Left, const TMatrix<Type, Inner, Columns> &Right ) noexcept;

    /** @brief Matrix Multiplication with rvalue reference for the right operand */
    template <typename Type, USize Rows, USize Inner, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &Left, const TMatrix<Type, Inner, Columns> &&Right ) noexcept;

} // namespace Maths

} // namespace LumenEngine

#include "Inline/Matrix.inl"
