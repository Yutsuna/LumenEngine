/**
 * @file Matrix.hpp
 * @brief Declaration of the TMatrix struct for mathematical matrix operations.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "Maths/Vec.hpp"

#include <type_traits>

namespace LumenEngine
{

namespace Maths
{

    namespace Concepts
    {

        template <USize Rows, USize Columns>
        concept CMatrixDimension = Rows > 0 && Rows <= 4 && Columns > 0 && Columns <= 4;

        template <USize Rows, USize Columns>
        concept CSquareMatrix = CMatrixDimension<Rows, Columns> && ( Rows == Columns );

        template <typename Type>
        concept CFloatingPoint = std::is_floating_point_v<Type>;

    } // namespace Concepts

    /**
     * @struct TMatrix
     * @brief A column-major matrix template.
     * @tparam Type The scalar type (Float32, Float64, etc.)
     * @tparam Rows Number of rows
     * @tparam Columns Number of columns
     */
    template <typename Type, USize Rows, USize Columns>
        requires Concepts::CMatrixDimension<Rows, Columns>
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

    public:

        /** @brief Access a column by index */
        constexpr ColumnType &operator[]( USize ColumnIndex ) noexcept;
        /** @brief Access a column by index */
        constexpr const ColumnType &operator[]( USize ColumnIndex ) const noexcept;

        /** @brief Extracts a specific row as a vector */
        [[nodiscard]] constexpr RowType GetRow ( USize RowIndex ) const noexcept;

        constexpr Bool operator==( const TMatrix<Type, Rows, Columns> &Other ) const noexcept;
        constexpr Bool operator!=( const TMatrix<Type, Rows, Columns> &Other ) const noexcept;

    public:

        /** @brief Returns an identity matrix. Only valid for square matrices. */
        static constexpr TMatrix<Type, Rows, Columns> Identity () noexcept
            requires Concepts::CSquareMatrix<Rows, Columns>;

        /** @brief Create a translation matrix */
        static constexpr TMatrix<Type, 4, 4> Translate ( const TVec<Type, 3> &InTranslation ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create a scaling matrix */
        static constexpr TMatrix<Type, 4, 4> Scale ( const TVec<Type, 3> &InScale ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create a rotation matrix around X axis (radians) */
        static constexpr TMatrix<Type, 4, 4> RotateX ( Type InAngleRadians ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create a rotation matrix around Y axis (radians) */
        static constexpr TMatrix<Type, 4, 4> RotateY ( Type InAngleRadians ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create a rotation matrix around Z axis (radians) */
        static constexpr TMatrix<Type, 4, 4> RotateZ ( Type InAngleRadians ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create a perspective projection matrix */
        static constexpr TMatrix<Type, 4, 4> Perspective ( Type InFieldOfViewRadians, Type InAspectRatio, Type InNearPlane, Type InFarPlane ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create an orthographic projection matrix (Vulkan clip space) */
        static constexpr TMatrix<Type, 4, 4> Orthographic ( Type InWidth, Type InHeight, Type InNearPlane, Type InFarPlane ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;

        /** @brief Create a look-at view matrix */
        static constexpr TMatrix<Type, 4, 4> LookAt ( const TVec<Type, 3> &InEye, const TVec<Type, 3> &InTarget, const TVec<Type, 3> &InUp ) noexcept
            requires Concepts::CSquareMatrix<4, 4>;
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

    /** @brief Matrix Multiplication with rvalue reference for the left operand */
    template <typename Type, USize Rows, USize Inner, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &&Left, const TMatrix<Type, Inner, Columns> &Right ) noexcept;

    /** @brief Matrix Multiplication with rvalue reference for both operands */
    template <typename Type, USize Rows, USize Inner, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &&Left, const TMatrix<Type, Inner, Columns> &&Right ) noexcept;

    /** @brief Matrix Multiplication with a scalar */
    template <typename Type, USize Rows, USize Columns>
    constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Columns> &Left, const Type Scalar ) noexcept;

    /**
     * @brief Compute a conservative world-space AABB by transforming the 8
     *        corners of the object-space AABB through the world transform.
     */
    void TransformAABB ( const FVec3f &LocalMin, const FVec3f &LocalMax, const FMatrix4x4f &Transform, FVec3f &OutWorldMin, FVec3f &OutWorldMax ) noexcept;

} // namespace Maths

} // namespace LumenEngine

#include "Inline/Matrix.inl"
