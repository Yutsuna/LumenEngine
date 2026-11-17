/**
 * @file RHIObjectBase.hpp
 * @brief Declares the reusable implementation of IRHIObject.
 */
#pragma once

#include <Core/CoreDefinitions.hpp>
#include <HAL/PlatformMutex.hpp>
#include <RHIExport.hpp>
#include <Templates/Function.hpp>
#include <Types/IRHIObject.hpp>

namespace Lumen
{

/**
 * @class FRHIObjectBase
 * @brief Optional backend base providing identity, names, and tags.
 */
class RHI_EXPORT FRHIObjectBase : public IRHIObject
{
public:

    /** @brief Creates an object with a stable identifier and concrete type tag. */
    FRHIObjectBase ( FRHIObjectId InId, UInt64 InBackendTypeTag ) noexcept;
    ~FRHIObjectBase () noexcept override;

    /**
     * @brief Returns the immutable object identifier.
     * @return The stable identifier.
     */
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    /**
     * @brief Returns a copy of the debug name.
     * @return The current debug name.
     */
    [[nodiscard]] FString GetDebugName () const noexcept override;
    /**
     * @brief Sets the debug name under the internal lock.
     * @param InName Name copied into owned storage.
     */
    void SetDebugName ( FStringView InName ) noexcept override;
    /**
     * @brief Returns the concrete backend type tag.
     * @return The tag used by backend casts.
     */
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    /** @brief Installs the non-owning registry callback invoked during destruction. */
    void SetDestructionCallback ( TFunction<void()> InCallback ) noexcept;

private:

    FRHIObjectId Id;
    UInt64 BackendTypeTag;
    FString DebugName;
    TFunction<void()> DestructionCallback;
    mutable HAL::FPlatformRecursiveSharedMutex DebugNameMutex;
};

} // namespace Lumen

#include "Inline/RHIObjectBase.inl"
