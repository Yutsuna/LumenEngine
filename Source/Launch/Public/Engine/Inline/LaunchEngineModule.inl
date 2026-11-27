/**
 * @file LaunchEngineModule.inl
 * @brief Inline and template implementation of LaunchEngineModule.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Engine/LaunchEngineModule.hpp>

#include <Concepts/SameAs.hpp>
#include <Meta/Reflection.hpp>

#include <array>
#include <iterator>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace Launch
{

    namespace Detail
    {

        template <typename> inline constexpr Bool TAlwaysFalse = false;

        template <typename TType> [[nodiscard]] consteval FStringView CanonicalTypeName () noexcept
        {
#if defined( __clang__ ) or defined( __GNUC__ )
            return __PRETTY_FUNCTION__;
#elif defined( _MSC_VER )
            return __FUNCSIG__;
#else
            return Meta::TypeName<std::remove_cvref_t<TType>>();
#endif
        }

        [[nodiscard]] consteval FModuleId HashTypeName ( const FStringView InName ) noexcept
        {
            UInt64 Hash = 14695981039346656037ULL;
            for ( const AnsiChar Ch : InName )
            {
                Hash ^= static_cast<UInt8>( Ch );
                Hash *= 1099511628211ULL;
            }
            return Hash;
        }

        template <typename TType> [[nodiscard]] consteval FStringView QualifiedModuleNameOf () noexcept
        {
            return CanonicalTypeName<std::remove_cvref_t<TType>>();
        }

        template <typename TType> [[nodiscard]] consteval FModuleId ModuleIdOf () noexcept
        {
            return HashTypeName( QualifiedModuleNameOf<TType>() );
        }

        template <typename TType> [[nodiscard]] consteval FStringView ModuleNameOf () noexcept
        {
#if LUMEN_HAS_REFLECTION
            return Meta::TypeName<std::remove_cvref_t<TType>>();
#else
            return QualifiedModuleNameOf<TType>();
#endif
        }

        template <typename TTarget, typename TList> struct TListContains : std::false_type
        {
        };

        template <typename TTarget, typename... Ts>
        struct TListContains<TTarget, TModuleList<Ts...>>
            : std::bool_constant<( ::Lumen::Concepts::CSameAs<std::remove_cvref_t<TTarget>, std::remove_cvref_t<Ts>> or ... )>
        {
        };

        template <typename TModule> struct TGetRequires
        {
            using Type = TModuleList<>;
        };
        template <typename TModule>
            requires requires { typename TModule::FRequires; }
        struct TGetRequires<TModule>
        {
            using Type = typename TModule::FRequires;
        };

        template <typename TModule> struct TGetOptionalRequires
        {
            using Type = TModuleList<>;
        };
        template <typename TModule>
            requires requires { typename TModule::FOptionalRequires; }
        struct TGetOptionalRequires<TModule>
        {
            using Type = typename TModule::FOptionalRequires;
        };

        template <typename TModule> struct TGetBefore
        {
            using Type = TModuleList<>;
        };
        template <typename TModule>
            requires requires { typename TModule::FBefore; }
        struct TGetBefore<TModule>
        {
            using Type = typename TModule::FBefore;
        };

        template <typename TModule> struct TGetProvides
        {
            using Type = TModuleList<>;
        };
        template <typename TModule>
            requires requires { typename TModule::FProvides; }
        struct TGetProvides<TModule>
        {
            using Type = typename TModule::FProvides;
        };

        template <typename TModule, typename TService> [[nodiscard]] void *UpcastService ( IEngineModule *const InBase ) noexcept
        {
            static_assert( std::is_base_of_v<TService, TModule>,
                           "Module must derive from every service listed in FProvides." );
            return static_cast<TService *>( static_cast<TModule *>( InBase ) );
        }

        template <typename... Ts> struct TDepStorage
        {
            static constexpr std::array<FModuleDep, sizeof...( Ts )> Items = {
                FModuleDep{
                    .Id            = ModuleIdOf<Ts>(),
                    .Name          = ModuleNameOf<Ts>(),
                    .QualifiedName = QualifiedModuleNameOf<Ts>(),
                }...,
            };
        };

        template <typename TModule, typename... TServices> struct TProvideStorage
        {
            static constexpr std::array<FServiceProvider, 1 + sizeof...( TServices )> Items = {
                FServiceProvider{
                    .Id            = ModuleIdOf<TModule>(),
                    .Name          = ModuleNameOf<TModule>(),
                    .QualifiedName = QualifiedModuleNameOf<TModule>(),
                    .Upcast        = &UpcastService<TModule, TModule>,
                },
                FServiceProvider{
                    .Id            = ModuleIdOf<TServices>(),
                    .Name          = ModuleNameOf<TServices>(),
                    .QualifiedName = QualifiedModuleNameOf<TServices>(),
                    .Upcast        = &UpcastService<TModule, TServices>,
                }...,
            };
        };

        template <typename... Ts> [[nodiscard]] consteval TSpan<const FModuleDep> MakeDepSpan ( TModuleList<Ts...> ) noexcept
        {
            if constexpr ( sizeof...( Ts ) == 0 )
            {
                return {};
            }
            else
            {
                return TSpan<const FModuleDep>( TDepStorage<Ts...>::Items.data(), TDepStorage<Ts...>::Items.size() );
            }
        }

        template <typename TModule, typename... TServices>
        [[nodiscard]] consteval TSpan<const FServiceProvider> MakeProvideSpan ( TModuleList<TServices...> ) noexcept
        {
            return TSpan<const FServiceProvider>( TProvideStorage<TModule, TServices...>::Items.data(),
                                                  TProvideStorage<TModule, TServices...>::Items.size() );
        }

        template <typename TModule> [[nodiscard]] consteval TSpan<const FStringView> ResolveCliOptions () noexcept
        {
            if constexpr ( requires {
                               std::data( TModule::CliOptions );
                               std::size( TModule::CliOptions );
                           } )
            {
                return TSpan<const FStringView>( std::data( TModule::CliOptions ), std::size( TModule::CliOptions ) );
            }
            else
            {
                return {};
            }
        }

        template <typename TModule> [[nodiscard]] consteval ETickPhase ResolveTickPhase () noexcept
        {
            if constexpr ( requires { TModule::TickPhase; } )
            {
                return TModule::TickPhase;
            }
            else
            {
                return ETickPhase::Update;
            }
        }

        template <typename TModule> [[nodiscard]] consteval Int32 ResolveLoadPriority () noexcept
        {
            if constexpr ( requires { TModule::LoadPriority; } )
            {
                return TModule::LoadPriority;
            }
            else
            {
                return 0;
            }
        }

        template <typename TModule>
        [[nodiscard]] Bool DefaultIsEnabled ( const FLaunchEngineConfig &InConfig, const FCommandLine &InCommandLine ) noexcept
        {
            if constexpr ( requires {
                               { TModule::IsEnabled( InConfig, InCommandLine ) } -> ::Lumen::Concepts::CSameAs<Bool>;
                           } )
            {
                return TModule::IsEnabled( InConfig, InCommandLine );
            }
            else if constexpr ( requires {
                                    { TModule::IsEnabled( InConfig ) } -> ::Lumen::Concepts::CSameAs<Bool>;
                                } )
            {
                static_cast<void>( InCommandLine );
                return TModule::IsEnabled( InConfig );
            }
            else
            {
                static_cast<void>( InConfig );
                static_cast<void>( InCommandLine );
                return true;
            }
        }

        template <typename TModule> [[nodiscard]] FModuleResult DefaultCreate ( const FModuleContext &InContext ) noexcept
        {
            if constexpr ( requires( const TModuleContext<TModule> &Typed ) { TModule::Create( Typed ); } )
            {
                const TModuleContext<TModule> TypedContext( InContext );
                return TModule::Create( TypedContext ).Transform( [] ( auto &&InPtr ) noexcept -> TUniquePtr<IEngineModule>
                {
                    return TUniquePtr<IEngineModule>( std::move( InPtr ) );
                } );
            }
            else if constexpr ( std::is_constructible_v<TModule, const TModuleContext<TModule> &> )
            {
                const TModuleContext<TModule> TypedContext( InContext );
                return TUniquePtr<IEngineModule>( MakeUnique<TModule>( TypedContext ) );
            }
            else if constexpr ( std::is_default_constructible_v<TModule> )
            {
                static_cast<void>( InContext );
                return TUniquePtr<IEngineModule>( MakeUnique<TModule>() );
            }
            else
            {
                static_cast<void>( InContext );
                static_assert( TAlwaysFalse<TModule>,
                               "Module must define Create(const TModuleContext<Self>&), "
                               "a constructor taking const TModuleContext<Self>&, or a default constructor." );
            }
        }

        template <typename TModule> struct TModuleStorage
        {
            static_assert( std::is_base_of_v<IEngineModule, TModule>,
                           "LUMEN_REGISTER_MODULE requires a type derived from IEngineModule." );
            static_assert( not TListContains<TModule, typename TGetRequires<TModule>::Type>::value,
                           "A module cannot declare itself in FRequires." );

            static inline constinit FModuleDescriptor Descriptor = {
                .Id               = ModuleIdOf<TModule>(),
                .Name             = ModuleNameOf<TModule>(),
                .QualifiedName    = QualifiedModuleNameOf<TModule>(),
                .Provides         = MakeProvideSpan<TModule>( typename TGetProvides<TModule>::Type{} ),
                .Requires         = MakeDepSpan( typename TGetRequires<TModule>::Type{} ),
                .OptionalRequires = MakeDepSpan( typename TGetOptionalRequires<TModule>::Type{} ),
                .Before           = MakeDepSpan( typename TGetBefore<TModule>::Type{} ),
                .CliOptions       = ResolveCliOptions<TModule>(),
                .TickPhase        = ResolveTickPhase<TModule>(),
                .LoadPriority     = ResolveLoadPriority<TModule>(),
                .IsEnabled        = &DefaultIsEnabled<TModule>,
                .Create           = &DefaultCreate<TModule>,
            };
        };

    } // namespace Detail

    template <typename TModule>
    TModuleContext<TModule>::TModuleContext ( const FModuleContext &InInner ) noexcept : Inner( InInner )
    {
        /** Empty */
    }

    template <typename TModule>
    template <typename TDep>
    TDep &TModuleContext<TModule>::Get () const noexcept
    {
        using FRequires = typename Detail::TGetRequires<TModule>::Type;
        static_assert( Detail::TListContains<TDep, FRequires>::value,
                       "Declare TDep in TModule::FRequires before calling InContext.Get<TDep>()." );

        void *const Ptr = Inner.FindRaw( Detail::ModuleIdOf<TDep>() );
        return *static_cast<TDep *>( Ptr );
    }

    template <typename TModule>
    template <typename TDep>
    TDep *TModuleContext<TModule>::Find () const noexcept
    {
        using FRequires = typename Detail::TGetRequires<TModule>::Type;
        using FOptional = typename Detail::TGetOptionalRequires<TModule>::Type;
        static_assert( Detail::TListContains<TDep, FRequires>::value or Detail::TListContains<TDep, FOptional>::value,
                       "Declare TDep in FRequires or FOptionalRequires before calling InContext.Find<TDep>()." );

        return static_cast<TDep *>( Inner.FindRaw( Detail::ModuleIdOf<TDep>() ) );
    }

    template <typename TModule> const FLaunchEngineConfig &TModuleContext<TModule>::GetConfig () const noexcept
    {
        return Inner.GetConfig();
    }

    template <typename TModule> const FCommandLine &TModuleContext<TModule>::GetCommandLine () const noexcept
    {
        return Inner.GetCommandLine();
    }

} // namespace Launch

} // namespace Lumen

/**
 * Registration
 */

#define LUMEN_REGISTER_MODULE_DETAIL_CONCAT_INNER( Left, Right ) Left##Right
#define LUMEN_REGISTER_MODULE_DETAIL_CONCAT( Left, Right ) LUMEN_REGISTER_MODULE_DETAIL_CONCAT_INNER( Left, Right )

#define LUMEN_REGISTER_MODULE_DETAIL_IMPL( ModuleType, UniqueId )                                                                \
    namespace                                                                                                                    \
    {                                                                                                                            \
        [[maybe_unused]] const ::Lumen::Int32 LUMEN_REGISTER_MODULE_DETAIL_CONCAT( GAutoRegisterModule_, UniqueId ) =            \
            [] () noexcept -> ::Lumen::Int32                                                                                     \
        {                                                                                                                        \
            ::Lumen::Launch::RegisterModuleDescriptor(                                                                           \
                ::Lumen::Launch::Detail::TModuleStorage<ModuleType>::Descriptor );                                               \
            return 0;                                                                                                            \
        }();                                                                                                                     \
    } // namespace

/**
 * @brief Registers a module descriptor at static initialization time (SIOF-safe, zero heap allocation).
 * @param ModuleType Concrete class derived from `Lumen::Launch::IEngineModule`.
 */
#define LUMEN_REGISTER_MODULE( ModuleType ) LUMEN_REGISTER_MODULE_DETAIL_IMPL( ModuleType, __COUNTER__ )
