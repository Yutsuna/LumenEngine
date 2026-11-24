/**
 * @file RHIObjectForward.hpp
 * @brief Provides public forwarding macros for concrete objects composing FRHIObjectBase.
 */
#pragma once

#include <RHI/Types/RHIObjectBase.hpp>

#define LUMEN_RHI_OBJECT_DECLARATIONS                                                                                            \
    [[nodiscard]] RHI::FRHIObjectId GetId() const noexcept override;                                                             \
    [[nodiscard]] Lumen::FString GetDebugName() const noexcept override;                                                         \
    void SetDebugName( Lumen::FStringView InName ) noexcept override;                                                            \
    [[nodiscard]] Lumen::UInt64 GetBackendTypeTag() const noexcept override;                                                     \
    [[nodiscard]] RHI::FRHIObjectBase &GetRHIObjectBase() noexcept;                                                              \
    [[nodiscard]] const RHI::FRHIObjectBase &GetRHIObjectBase() const noexcept

#define LUMEN_RHI_OBJECT_FORWARDERS( ModuleNamespace, Class )                                                                    \
    Lumen::RHI::FRHIObjectId Lumen::ModuleNamespace::Class::GetId() const noexcept                                               \
    {                                                                                                                            \
        return Identity.GetId();                                                                                                 \
    }                                                                                                                            \
    Lumen::FString Lumen::ModuleNamespace::Class::GetDebugName() const noexcept                                                  \
    {                                                                                                                            \
        return Identity.GetDebugName();                                                                                          \
    }                                                                                                                            \
    void Lumen::ModuleNamespace::Class::SetDebugName( const FStringView InName ) noexcept                                        \
    {                                                                                                                            \
        Identity.SetDebugName( InName );                                                                                         \
    }                                                                                                                            \
    Lumen::UInt64 Lumen::ModuleNamespace::Class::GetBackendTypeTag() const noexcept                                              \
    {                                                                                                                            \
        return Identity.GetBackendTypeTag();                                                                                     \
    }                                                                                                                            \
    Lumen::RHI::FRHIObjectBase &Lumen::ModuleNamespace::Class::GetRHIObjectBase() noexcept                                       \
    {                                                                                                                            \
        return Identity;                                                                                                         \
    }                                                                                                                            \
    const Lumen::RHI::FRHIObjectBase &Lumen::ModuleNamespace::Class::GetRHIObjectBase() const noexcept                           \
    {                                                                                                                            \
        return Identity;                                                                                                         \
    }
