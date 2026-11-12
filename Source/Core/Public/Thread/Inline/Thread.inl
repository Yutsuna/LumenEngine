/**
 * @file Thread.inl
 * @brief Inline implementation of FThread construction.
 */

#pragma once

#include <Thread/Thread.hpp>

#include <Memory/UniquePtr.hpp>
#include <Templates/Invoke.hpp>

#include <tuple>
#include <utility>

namespace Lumen
{

namespace Internal
{

    /**
     * @class TThreadData
     * @brief Holds the data for a thread, including the callable and arguments.
     */
    template <typename TFunction, typename... TArgs> class TThreadData final
    {
    public:

        template <typename TForwardedFunction, typename... TForwardedArgs>
        explicit TThreadData( FStopToken InStopToken, TForwardedFunction &&InFunction, TForwardedArgs &&...InArgs ) noexcept
            : StopToken( std::move( InStopToken ) ), Function( std::forward<TForwardedFunction>( InFunction ) ),
              Arguments( std::forward<TForwardedArgs>( InArgs )... )
        {
            /* */
        }

#if defined( LUMEN_PLATFORM_WINDOWS )
        static DWORD WINAPI Entry ( void *InData ) noexcept
#else
        static void *Entry ( void *InData ) noexcept
#endif
        {
            TUniquePtr<TThreadData> Data( static_cast<TThreadData *>( InData ) );
            Data->Run();
#if defined( LUMEN_PLATFORM_WINDOWS )
            return 0;
#else
            return nullptr;
#endif
        }

    private:

        void Run () noexcept
        {
            RunSequence( std::index_sequence_for<TArgs...>{} );
        }

        template <USize... Indices> void RunSequence ( std::index_sequence<Indices...> ) noexcept
        {
            if constexpr ( Concepts::CIsInvocable<TFunction, FStopToken, TArgs...> )
            {
                static_cast<void>(
                    Invoke( std::move( Function ), FStopToken( StopToken ), std::move( std::get<Indices>( Arguments ) )... ) );
            }
            else
            {
                static_cast<void>( Invoke( std::move( Function ), std::move( std::get<Indices>( Arguments ) )... ) );
            }
        }

    private:

        FStopToken StopToken;
        TFunction Function;
        std::tuple<TArgs...> Arguments;
    };

} // namespace Internal

template <typename TFunction, typename... TArgs>
    requires( not Concepts::CSameAs<std::remove_cvref_t<TFunction>, FThread> ) and
            ( Concepts::CIsInvocable<std::decay_t<TFunction>, FStopToken, std::decay_t<TArgs>...> or
              Concepts::CIsInvocable<std::decay_t<TFunction>, std::decay_t<TArgs>...> )
FThread::FThread( TFunction &&InFunction, TArgs &&...InArgs ) noexcept
{
    StopSource     = FStopSource();
    using DataType = Internal::TThreadData<std::decay_t<TFunction>, std::decay_t<TArgs>...>;
    TUniquePtr<DataType> Data( new ( std::nothrow ) DataType( StopSource.GetToken(), std::forward<TFunction>( InFunction ),
                                                              std::forward<TArgs>( InArgs )... ) );
    if ( not Data.IsValid() or not Thread.Start( &DataType::Entry, Data.Get() ) )
    {
        StopSource = FStopSource( nullptr );
        return;
    }
    static_cast<void>( Data.Release() );
}

} // namespace Lumen
