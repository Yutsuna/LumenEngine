/**
 * @file OpenGLObject.hpp
 * @brief Declares backend object identity and IRHIObject forwarding helpers for RHIOpenGL.
 */

#pragma once

#include <Core/CoreTypes.hpp>

#include <RHI/Types/RHIIdentifiers.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @return Stable backend type tag matching TryRHIBackendCast<TObjectType>.
     * @tparam TObjectType The backend object type to get the tag for.
     */
    template <class TObjectType> [[nodiscard]] constexpr UInt64 GetOpenGLBackendTypeTag () noexcept;

    /**
     * @brief Make a FRHIObjectBase bound to a concrete RHIOpenGL object type tag.
     * @tparam TObjectType The backend object type to create the FRHIObjectBase for.
     */
    template <class TObjectType> [[nodiscard]] RHI::FRHIObjectBase MakeOpenGLObjectBase ( RHI::FRHIObjectId InId ) noexcept;

} // namespace RHIOpenGL

} // namespace Lumen

#include "Inline/OpenGLObject.inl"
