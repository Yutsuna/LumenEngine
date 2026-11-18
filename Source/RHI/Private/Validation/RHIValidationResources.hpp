/**
 * @file RHIValidationResources.hpp
 * @brief Declares resource decorators and mapping/lifetime checks.
 */

#pragma once

#include "RHIValidationState.hpp"
#include <Resources/IRHISampler.hpp>
#include <Resources/IRHITextureView.hpp>

namespace Lumen
{

/**
 * @class FRHIValidationBuffer
 * @brief Validates buffer ownership and CPU mapping scopes.
 */
class FRHIValidationBuffer final : public IRHIBuffer, public IRHIValidationObject
{
public:

    FRHIValidationBuffer ( TSharedRef<IRHIBuffer> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationBuffer () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHIBufferDesc &GetDesc () const noexcept override;
    [[nodiscard]] const FRHIResourceRequirements &GetRequirements () const noexcept override;
    [[nodiscard]] ERHIResourceUsage GetUsage () const noexcept override;
    [[nodiscard]] ERHIResourceState GetInitialState () const noexcept override;
    [[nodiscard]] FRHIResourceLifetime GetLifetime () const noexcept override;
    [[nodiscard]] TExpected<TSpan<Byte>, FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
    void Unmap () noexcept override;

private:

    TSharedRef<IRHIBuffer> Underlying;
    TSharedPtr<FRHIValidationState> State;
    Bool bMapped = false;
};

/**
 * @class FRHIValidationTexture
 * @brief Tracks a texture and all its subresource states.
 */
class FRHIValidationTexture final : public IRHITexture, public IRHIValidationObject
{
public:

    FRHIValidationTexture ( TSharedRef<IRHITexture> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationTexture () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHITextureDesc &GetDesc () const noexcept override;
    [[nodiscard]] const FRHIResourceRequirements &GetRequirements () const noexcept override;
    [[nodiscard]] ERHIResourceUsage GetUsage () const noexcept override;
    [[nodiscard]] ERHIResourceState GetInitialState () const noexcept override;
    [[nodiscard]] FRHIResourceLifetime GetLifetime () const noexcept override;

private:

    TSharedRef<IRHITexture> Underlying;
    TSharedPtr<FRHIValidationState> State;
};

/**
 * @class FRHIValidationTextureView
 * @brief Preserves the validated parent texture ownership chain.
 */
class FRHIValidationTextureView final : public IRHITextureView, public IRHIValidationObject
{
public:

    FRHIValidationTextureView ( TSharedRef<IRHITextureView> InUnderlying,
                                TSharedPtr<IRHITexture> InParent,
                                TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationTextureView () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHITextureViewDesc &GetDesc () const noexcept override;
    [[nodiscard]] TSharedPtr<IRHITexture> GetTexture () const noexcept override;

private:

    TSharedRef<IRHITextureView> Underlying;
    TSharedPtr<IRHITexture> Parent;
    TSharedPtr<FRHIValidationState> State;
};

/**
 * @class FRHIValidationSampler
 * @brief Forwards immutable sampler behavior while tracking lifetime.
 */
class FRHIValidationSampler final : public IRHISampler, public IRHIValidationObject
{
public:

    FRHIValidationSampler ( TSharedRef<IRHISampler> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationSampler () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHISamplerDesc &GetDesc () const noexcept override;

private:

    TSharedRef<IRHISampler> Underlying;
    TSharedPtr<FRHIValidationState> State;
};

} // namespace Lumen
