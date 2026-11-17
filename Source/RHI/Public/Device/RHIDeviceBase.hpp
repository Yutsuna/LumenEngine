/**
 * @file RHIDeviceBase.hpp
 * @brief Declares reusable device status, identity, and child tracking.
 */
#pragma once

#include <Device/IRHIDevice.hpp>
#include <HAL/PlatformMutex.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIObjectBase.hpp>

namespace Lumen
{

/**
 * @class FRHIDeviceBase
 * @brief Optional backend base implementing common non-native behavior.
 */
class RHI_EXPORT FRHIDeviceBase : public IRHIDevice
{
public:

    ~FRHIDeviceBase () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] const FRHICapabilities &GetCapabilities () const noexcept override;
    [[nodiscard]] const FRHILimits &GetLimits () const noexcept override;
    [[nodiscard]] ERHIDeviceStatus GetStatus () const noexcept override;

    /** @return Number of currently registered live children. */
    [[nodiscard]] USize GetTrackedObjectCount () const noexcept;

protected:

    /** @brief Initializes negotiated values and the concrete backend tag. */
    FRHIDeviceBase ( FRHICapabilities InCapabilities, FRHILimits InLimits, UInt64 InBackendTypeTag ) noexcept;
    /** @brief Changes status monotonically from ready to lost/closed. */
    void SetStatus ( ERHIDeviceStatus InStatus ) noexcept;
    /** @return A fresh generic identifier for a child object. */
    [[nodiscard]] FRHIObjectId NextObjectId () noexcept;
    /** @brief Adds a live child and registers its destruction notification. */
    template <typename ObjectType>
        requires requires( ObjectType &Object ) { Object.GetRHIObjectBase(); }
    void TrackObject ( ObjectType &InObject ) noexcept
    {
        TrackObject( static_cast<IRHIObject &>( InObject ), InObject.GetRHIObjectBase() );
    }
    /** @brief Adds an object directly derived from FRHIObjectBase. */
    void TrackObject ( FRHIObjectBase &InObject ) noexcept
    {
        TrackObject( static_cast<IRHIObject &>( InObject ), InObject );
    }
    /** @brief Removes a child during its destruction. */
    void UntrackObject ( IRHIObject &InObject ) noexcept;
    /** @return DeviceLost/DeviceClosed error when the device is not ready. */
    [[nodiscard]] TOptional<FRHIError> GetUnavailableError ( FStringView InOperation ) const noexcept;

private:

    struct FObjectRegistry
    {
        TVector<IRHIObject *> Objects;
        HAL::FPlatformRecursiveSharedMutex Mutex;
    };

    void TrackObject ( IRHIObject &InObject, FRHIObjectBase &InBase ) noexcept;

private:

    FRHIObjectBase Identity;
    FRHICapabilities Capabilities;
    FRHILimits Limits;
    TAtomic<ERHIDeviceStatus> Status;
    FRHIIdentifierGenerator Identifiers;
    TSharedPtr<FObjectRegistry> Registry;
};

} // namespace Lumen
