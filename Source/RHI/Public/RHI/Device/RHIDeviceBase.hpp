/**
 * @file RHIDeviceBase.hpp
 * @brief Declares reusable device status, identity, and child tracking.
 */
#pragma once

#include <HAL/PlatformMutex.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Types/RHIObjectBase.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIDeviceBase
     * @brief Optional backend base implementing common non-native behavior.
     */
    class RHI_EXPORT FRHIDeviceBase : public IRHIDevice
    {
    public:

        ~FRHIDeviceBase () noexcept override;
        /**
         * @brief Returns the device identifier.
         * @return The stable identifier.
         */
        [[nodiscard]] FRHIObjectId GetId () const noexcept override;
        /**
         * @brief Returns a copy of the debug name.
         * @return The current debug name.
         */
        [[nodiscard]] FString GetDebugName () const noexcept override;
        /**
         * @brief Sets the debug name under the object lock.
         * @param InName Name copied into owned storage.
         */
        void SetDebugName ( FStringView InName ) noexcept override;
        /**
         * @brief Returns the concrete backend type tag.
         * @return The tag used by backend casts.
         */
        [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
        /**
         * @brief Returns the negotiated capabilities.
         * @return Immutable capabilities owned by the device.
         */
        [[nodiscard]] const FRHICapabilities &GetCapabilities () const noexcept override;
        /**
         * @brief Returns the negotiated limits.
         * @return Immutable limits owned by the device.
         */
        [[nodiscard]] const FRHILimits &GetLimits () const noexcept override;
        /**
         * @brief Returns the current device status.
         * @return Ready, lost or closed status.
         */
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

        /** @brief Registers a child and its destruction callback under the registry lock. */
        void TrackObject ( IRHIObject &InObject, FRHIObjectBase &InBase ) noexcept;

    private:

        FRHIObjectBase Identity;
        FRHICapabilities Capabilities;
        FRHILimits Limits;
        TAtomic<ERHIDeviceStatus> Status;
        FRHIIdentifierGenerator Identifiers;
        TSharedPtr<FObjectRegistry> Registry;
    };

} // namespace RHI

} // namespace Lumen
