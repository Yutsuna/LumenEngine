/**
 * @file Matrix.hpp
 * @brief Lumen column-major matrix
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <Maths/Macros.hpp>
#include <Maths/Vec.hpp>
#include <MathsExport.hpp>

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

    /**
     * @struct TMatrix
     * @brief A column-major matrix template
     * @tparam Type The scalar type (Float32, Float64...)
     * @tparam Rows Number of rows
     * @tparam Columns Number of columns
     */
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

        constexpr TMatrix ( const Type &Scalar ) noexcept;
        constexpr TMatrix ( const ColumnType ( &InColumns )[Columns] ) noexcept;

        constexpr TMatrix ( const TMatrix &Other ) noexcept = default;
        constexpr TMatrix ( TMatrix &&Other ) noexcept      = default;

        constexpr TMatrix &operator=( const TMatrix &Other ) noexcept = default;
        constexpr TMatrix &operator=( TMatrix &&Other ) noexcept      = default;

        constexpr ColumnType &operator[]( USize ColumnIndex ) noexcept;
        constexpr const ColumnType &operator[]( USize ColumnIndex ) const noexcept;

        [[nodiscard]] constexpr RowType GetRow ( USize RowIndex ) const noexcept;

        constexpr Bool operator==( const TMatrix &Other ) const noexcept;
        constexpr Bool operator!=( const TMatrix &Other ) const noexcept;

        [[nodiscard]] static constexpr TMatrix Identity () noexcept
            requires Concepts::CSquareMatrix<Rows, Columns>;

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4> Translate ( const TVec<Type, 3> &InTranslation ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4> Scale ( const TVec<Type, 3> &InScale ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4> RotateX ( Type InAngleRadians ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4> RotateY ( Type InAngleRadians ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4> RotateZ ( Type InAngleRadians ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4>
        Perspective ( Type InFieldOfViewRadians, Type InAspectRatio, Type InNearPlane, Type InFarPlane ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4>
        Orthographic ( Type InWidth, Type InHeight, Type InNearPlane, Type InFarPlane ) noexcept
            requires( Rows == 4 and Columns == 4 );

        [[nodiscard]] static constexpr TMatrix<Type, 4, 4>
        LookAt ( const TVec<Type, 3> &InEye, const TVec<Type, 3> &InTarget, const TVec<Type, 3> &InUp ) noexcept
            requires( Rows == 4 and Columns == 4 );
    };

    /**
     * Operators
     */

    template <typename Type, USize Rows, USize Inner, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &Left,
                                                                    const TMatrix<Type, Inner, Columns> &Right ) noexcept;

    template <typename Type, USize Rows, USize Inner, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &Left,
                                                                    const TMatrix<Type, Inner, Columns> &&Right ) noexcept;

    template <typename Type, USize Rows, USize Inner, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &&Left,
                                                                    const TMatrix<Type, Inner, Columns> &Right ) noexcept;

    template <typename Type, USize Rows, USize Inner, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Inner> &&Left,
                                                                    const TMatrix<Type, Inner, Columns> &&Right ) noexcept;

    template <typename Type, USize Rows, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator*( const TMatrix<Type, Rows, Columns> &Left,
                                                                    const Type Scalar ) noexcept;

    template <typename Type, USize Rows, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator+( const TMatrix<Type, Rows, Columns> &Left,
                                                                    const TMatrix<Type, Rows, Columns> &Right ) noexcept;

    template <typename Type, USize Rows, USize Columns>
    [[nodiscard]] constexpr TMatrix<Type, Rows, Columns> operator+( const TMatrix<Type, Rows, Columns> &Left,
                                                                    const Type Scalar ) noexcept;

    /**
     * Aliases
     */

    using FMat2x2d = Maths::TMatrix<Float64, 2, 2>;
    using FMat2x3d = Maths::TMatrix<Float64, 2, 3>;
    using FMat3x2d = Maths::TMatrix<Float64, 3, 2>;
    using FMat3x3d = Maths::TMatrix<Float64, 3, 3>;
    using FMat3x4d = Maths::TMatrix<Float64, 3, 4>;
    using FMat4x4d = Maths::TMatrix<Float64, 4, 4>;

    using FMat2x2f = Maths::TMatrix<Float32, 2, 2>;
    using FMat2x3f = Maths::TMatrix<Float32, 2, 3>;
    using FMat3x2f = Maths::TMatrix<Float32, 3, 2>;
    using FMat3x3f = Maths::TMatrix<Float32, 3, 3>;
    using FMat3x4f = Maths::TMatrix<Float32, 3, 4>;
    using FMat4x4f = Maths::TMatrix<Float32, 4, 4>;

    using FMat2x2i = Maths::TMatrix<Int32, 2, 2>;
    using FMat2x3i = Maths::TMatrix<Int32, 2, 3>;
    using FMat3x2i = Maths::TMatrix<Int32, 3, 2>;
    using FMat3x3i = Maths::TMatrix<Int32, 3, 3>;
    using FMat3x4i = Maths::TMatrix<Int32, 3, 4>;
    using FMat4x4i = Maths::TMatrix<Int32, 4, 4>;

    using FMat2x2u = Maths::TMatrix<UInt32, 2, 2>;
    using FMat2x3u = Maths::TMatrix<UInt32, 2, 3>;
    using FMat3x2u = Maths::TMatrix<UInt32, 3, 2>;
    using FMat3x3u = Maths::TMatrix<UInt32, 3, 3>;
    using FMat3x4u = Maths::TMatrix<UInt32, 3, 4>;
    using FMat4x4u = Maths::TMatrix<UInt32, 4, 4>;

    /**
     * @brief Transform an axis-aligned bounding box by a matrix using Arvo's method.
     * @param LocalMin The minimum corner of the local-space AABB.
     * @param LocalMax The maximum corner of the local-space AABB.
     * @param Transform The transform to apply.
     * @param OutWorldMin The minimum corner of the transformed AABB.
     * @param OutWorldMax The maximum corner of the transformed AABB.
     */
    MATHS_EXPORT void TransformAABB ( const FVec3f &LocalMin,
                                      const FVec3f &LocalMax,
                                      const FMat4x4f &Transform,
                                      FVec3f &OutWorldMin,
                                      FVec3f &OutWorldMax ) noexcept;

    template <typename Type, USize TRow, USize TColumn>
    [[nodiscard]] Bool IsEqual ( const TMatrix<Type, TRow, TColumn> &MatA,
                                 const TMatrix<Type, TRow, TColumn> &MatB,
                                 Type InEpsilon = Epsilon<Type> ) noexcept;

} // namespace Maths

} // namespace Lumen

#include "Inline/Matrix.inl"
