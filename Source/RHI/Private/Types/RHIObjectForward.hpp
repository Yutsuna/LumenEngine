/**
 * @file RHIObjectForward.hpp
 * @brief Provides private forwarding macros for concrete objects composing FRHIObjectBase.
 */
#pragma once

#include <RHI/Types/RHIObjectBase.hpp>

#define LUMEN_RHI_OBJECT_DECLARATIONS                                                                                            \
    [[nodiscard]] FRHIObjectId GetId() const noexcept override;                                                                  \
    [[nodiscard]] FString GetDebugName() const noexcept override;                                                                \
    void SetDebugName( FStringView InName ) noexcept override;                                                                   \
    [[nodiscard]] UInt64 GetBackendTypeTag() const noexcept override;                                                            \
    [[nodiscard]] FRHIObjectBase &GetRHIObjectBase() noexcept

#define LUMEN_RHI_OBJECT_FORWARDERS( Class )                                                                                     \
    Lumen::RHI::FRHIObjectId Lumen::RHI::Class::GetId() const noexcept                                                           \
    {                                                                                                                            \
        return Identity.GetId();                                                                                                 \
    }                                                                                                                            \
    Lumen::FString Lumen::RHI::Class::GetDebugName() const noexcept                                                              \
    {                                                                                                                            \
        return Identity.GetDebugName();                                                                                          \
    }                                                                                                                            \
    void Lumen::RHI::Class::SetDebugName( const FStringView InName ) noexcept                                                    \
    {                                                                                                                            \
        Identity.SetDebugName( InName );                                                                                         \
    }                                                                                                                            \
    Lumen::UInt64 Lumen::RHI::Class::GetBackendTypeTag() const noexcept                                                          \
    {                                                                                                                            \
        return Identity.GetBackendTypeTag();                                                                                     \
    }                                                                                                                            \
    Lumen::RHI::FRHIObjectBase &Lumen::RHI::Class::GetRHIObjectBase() noexcept                                                   \
    {                                                                                                                            \
        return Identity;                                                                                                         \
    }
