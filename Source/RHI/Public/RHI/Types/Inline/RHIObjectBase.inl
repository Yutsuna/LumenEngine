/**
 * @file RHIObjectBase.inl
 * @brief Inline getters of FRHIObjectBase.
 */

#pragma once

#include <RHI/Types/RHIObjectBase.hpp>

inline Lumen::RHI::FRHIObjectId Lumen::RHI::FRHIObjectBase::GetId () const noexcept
{
    return Id;
}

inline Lumen::UInt64 Lumen::RHI::FRHIObjectBase::GetBackendTypeTag () const noexcept
{
    return BackendTypeTag;
}
