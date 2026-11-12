/**
 * @file StopToken.inl
 * @brief Inline implementation of FStopCallback.
 */

#pragma once

#include <Templates/Invoke.hpp>
#include <Templates/StopToken.hpp>

#include <new>
#include <utility>

namespace Lumen
{
namespace Internal
{
    template <typename TCallback> class TCallbackNode final : public FCallbackNode
    {
    public:

        template <typename TForwardedCallback>
        explicit TCallbackNode( TForwardedCallback &&InCallback ) noexcept
            : Callback( std::forward<TForwardedCallback>( InCallback ) )
        {
            /* */
        }

        void Execute () noexcept override
        {
            Invoke( Callback );
        }

    private:

        [[no_unique_address]] TCallback Callback;
    };
} // namespace Internal

template <typename TCallback>
    requires Concepts::CIsInvocable<std::decay_t<TCallback> &>
Lumen::FStopCallback::FStopCallback( const FStopToken &InStopToken, TCallback &&InCallback ) noexcept
{
    if ( InStopToken.State != nullptr )
    {
        Callback = new ( std::nothrow ) Internal::TCallbackNode<std::decay_t<TCallback>>( std::forward<TCallback>( InCallback ) );
        Register( InStopToken );
    }
}
} // namespace Lumen
