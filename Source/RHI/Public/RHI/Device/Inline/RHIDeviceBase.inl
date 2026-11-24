/**
 * @file RHIDeviceBase.inl
 * @brief Inline implementation of FRHIDeviceBase.
 */

#pragma once

#include <RHI/Device/RHIDeviceBase.hpp>

/**
 * Public
 */

template <class TObjectType>
    requires requires( TObjectType &Object ) { Object.GetRHIObjectBase(); }
void Lumen::RHI::FRHIDeviceBase::TrackObject ( TObjectType &InObject ) noexcept
{
    TrackObject( static_cast<IRHIObject &>( InObject ), InObject.GetRHIObjectBase() );
}

inline void Lumen::RHI::FRHIDeviceBase::TrackObject ( FRHIObjectBase &InObject ) noexcept
{
    TrackObject( static_cast<IRHIObject &>( InObject ), InObject );
}
