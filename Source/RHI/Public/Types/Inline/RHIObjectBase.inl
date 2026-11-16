/**
 * @file RHIObjectBase.inl
 * @brief Inline getters of FRHIObjectBase.
 */
#pragma once

#include <Types/RHIObjectBase.hpp>

inline Lumen::FRHIObjectId Lumen::FRHIObjectBase::GetId () const noexcept
{
    return Id;
}

inline Lumen::UInt64 Lumen::FRHIObjectBase::GetBackendTypeTag () const noexcept
{
    return BackendTypeTag;
}
