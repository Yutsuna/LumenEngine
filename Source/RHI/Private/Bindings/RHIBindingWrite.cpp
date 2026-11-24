/**
 * @file RHIBindingWrite.cpp
 * @brief Implements typed binding write validation.
 */

#include <RHI/Bindings/RHIBindingWrite.hpp>
#include <variant>

/**
 * Public
 */

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIBindingWrite::Validate () const noexcept
{
    const Bool bValid = std::visit( [] ( const auto &Payload ) noexcept -> Bool
    {
        if constexpr ( requires { Payload.Buffer; } )
        {
            return Payload.Buffer.IsValid();
        }
        else if constexpr ( requires {
                                Payload.View;
                                Payload.Sampler;
                            } )
        {
            return Payload.View.IsValid() and Payload.Sampler.IsValid();
        }
        else if constexpr ( requires { Payload.View; } )
        {
            return Payload.View.IsValid();
        }
        else
        {
            return Payload.Sampler.IsValid();
        }
    }, Resource );

    if ( not bValid )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Binding write contains no resource" ) );
    }
    return {};
}
