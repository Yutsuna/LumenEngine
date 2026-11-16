/**
 * @file RHIObjectForward.hpp
 * @brief Provides private forwarding macros for concrete objects composing FRHIObjectBase.
 */
#pragma once

#include <Types/RHIObjectBase.hpp>

#define LUMEN_RHI_OBJECT_DECLARATIONS                                                                                            \
    [[nodiscard]] FRHIObjectId GetId() const noexcept override;                                                                  \
    [[nodiscard]] FString GetDebugName() const noexcept override;                                                                \
    void SetDebugName( FStringView InName ) noexcept override;                                                                   \
    [[nodiscard]] UInt64 GetBackendTypeTag() const noexcept override

#define LUMEN_RHI_OBJECT_FORWARDERS( Class )                                                                                     \
    Lumen::FRHIObjectId Lumen::Class::GetId() const noexcept                                                                     \
    {                                                                                                                            \
        return Identity.GetId();                                                                                                 \
    }                                                                                                                            \
    Lumen::FString Lumen::Class::GetDebugName() const noexcept                                                                   \
    {                                                                                                                            \
        return Identity.GetDebugName();                                                                                          \
    }                                                                                                                            \
    void Lumen::Class::SetDebugName( const FStringView InName ) noexcept                                                         \
    {                                                                                                                            \
        Identity.SetDebugName( InName );                                                                                         \
    }                                                                                                                            \
    Lumen::UInt64 Lumen::Class::GetBackendTypeTag() const noexcept                                                               \
    {                                                                                                                            \
        return Identity.GetBackendTypeTag();                                                                                     \
    }
