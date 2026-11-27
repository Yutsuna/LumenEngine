/**
 * @file OpenGLObject.inl
 * @brief Inline implementation of OpenGLObject.hpp.
 */

#pragma once

#include <RHIOpenGL/Types/OpenGLObject.hpp>

#include <Templates/Hash.hpp>

template <class TObjectType> constexpr Lumen::UInt64 Lumen::RHIOpenGL::GetOpenGLBackendTypeTag () noexcept
{
    return FHash::TypeHash<TObjectType>();
}

template <typename TObjectType>
Lumen::RHI::FRHIObjectBase Lumen::RHIOpenGL::MakeOpenGLObjectBase ( RHI::FRHIObjectId InId ) noexcept
{
    return RHI::FRHIObjectBase( InId, GetOpenGLBackendTypeTag<TObjectType>() );
}
