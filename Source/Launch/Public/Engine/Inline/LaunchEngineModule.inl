/**
 * @file LaunchEngineModule.inl
 * @brief Inline and template implementation of LaunchEngineModule.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>
#include <Engine/LaunchEngineModule.hpp>

#include <Concepts/ConstructibleFrom.hpp>
#include <Concepts/SameAs.hpp>
#include <Meta/Reflection.hpp>
#include <Templates/Array.hpp>
#include <Templates/Hash.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace Launch
{

    namespace Internal
    {

        template <typename TType> [[nodiscard]] consteval FStringView CanonicalTypeName () noexcept
        {
#if defined( LUMEN_COMPILER_CLANG ) or defined( LUMEN_COMPILER_GCC )
            return __PRETTY_FUNCTION__;
#elif defined( LUMEN_COMPILER_MSVC )
            return __FUNCSIG__;
#else
            return Meta::TypeName<std::remove_cvref_t<TType>>();
#endif
        }

        template <typename TType> [[nodiscard]] consteval FStringView QualifiedModuleNameOf () noexcept
        {
            return CanonicalTypeName<std::remove_cvref_t<TType>>();
        }

        /**
         * @brief Module identity is Core's stable FNV-1a type hash: one implementation for the whole engine,
         *        no module-local duplicate and no cross-TU drift.
         */
        template <typename TType> [[nodiscard]] consteval FModuleId ModuleIdOf () noexcept
        {
            return FHash::TypeHash<std::remove_cvref_t<TType>>();
        }

        template <typename TType> [[nodiscard]] consteval FStringView ModuleNameOf () noexcept
        {
#if LUMEN_HAS_REFLECTION
            return Meta::TypeName<std::remove_cvref_t<TType>>();
#else
            return QualifiedModuleNameOf<TType>();
#endif
        }

        /** @brief Compile-time membership test built on Core's CSameAs concept. */
        template <typename TTarget, typename TList> inline constexpr Bool TListContainsV = false;

        template <typename TTarget, typename... Ts>
        inline constexpr Bool TListContainsV<TTarget, TModuleList<Ts...>> =
            ( ::Lumen::Concepts::CSameAs<std::remove_cvref_t<TTarget>, std::remove_cvref_t<Ts>> or ... );

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
            static_assert( std::is_base_of_v<TService, TModule>, "Module must derive from every service listed in FProvides." );
            return static_cast<TService *>( static_cast<TModule *>( InBase ) );
        }

        template <typename... Ts> struct TDepStorage
        {
            static constexpr TArray<FModuleDep, sizeof...( Ts )> Items = {
                FModuleDep{
                    .Id            = ModuleIdOf<Ts>(),
                    .Name          = ModuleNameOf<Ts>(),
                    .QualifiedName = QualifiedModuleNameOf<Ts>(),
                }...,
            };
        };

        template <typename TModule, typename... TServices> struct TProvideStorage
        {
            static constexpr TArray<FServiceProvider, 1 + sizeof...( TServices )> Items = {
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
                return TModule::Create( TypedContext )
                    .Transform( [] ( auto &&InPtr ) noexcept -> TUniquePtr<IEngineModule>
                {
                    return TUniquePtr<IEngineModule>( std::move( InPtr ) );
                } );
            }
            /** INFO: An empty argument list means default construction. */
            else if constexpr ( ::Lumen::Concepts::CConstructibleFrom<TModule, const TModuleContext<TModule> &> )
            {
                const TModuleContext<TModule> TypedContext( InContext );
                return TUniquePtr<IEngineModule>( MakeUnique<TModule>( TypedContext ) );
            }
            else if constexpr ( ::Lumen::Concepts::CConstructibleFrom<TModule> )
            {
                static_cast<void>( InContext );
                return TUniquePtr<IEngineModule>( MakeUnique<TModule>() );
            }
            else
            {
                static_cast<void>( InContext );
                static_assert( AlwaysFalseV<TModule>,
                               "Module must define Create(const TModuleContext<Self>&), "
                               "a constructor taking const TModuleContext<Self>&, or a default constructor." );
            }
        }

        template <typename TModule> struct TModuleStorage
        {
            static_assert( std::is_base_of_v<IEngineModule, TModule>,
                           "LUMEN_REGISTER_MODULE requires a type derived from IEngineModule." );
            static_assert( not TListContainsV<TModule, typename TGetRequires<TModule>::Type>,
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

    } // namespace Internal

    template <typename TModule>
    TModuleContext<TModule>::TModuleContext ( const FModuleContext &InInner ) noexcept : Inner( InInner )
    {
        /** Empty */
    }

    template <typename TModule> template <typename TDep> TDep &TModuleContext<TModule>::Get () const noexcept
    {
        using FRequires = typename Internal::TGetRequires<TModule>::Type;
        static_assert( Internal::TListContainsV<TDep, FRequires>,
                       "Declare TDep in TModule::FRequires before calling InContext.Get<TDep>()." );

        void *const Ptr = Inner.FindRaw( Internal::ModuleIdOf<TDep>() );
        return *static_cast<TDep *>( Ptr );
    }

    template <typename TModule> template <typename TDep> TDep *TModuleContext<TModule>::Find () const noexcept
    {
        using FRequires = typename Internal::TGetRequires<TModule>::Type;
        using FOptional = typename Internal::TGetOptionalRequires<TModule>::Type;
        static_assert( Internal::TListContainsV<TDep, FRequires> or Internal::TListContainsV<TDep, FOptional>,
                       "Declare TDep in FRequires or FOptionalRequires before calling InContext.Find<TDep>()." );

        return static_cast<TDep *>( Inner.FindRaw( Internal::ModuleIdOf<TDep>() ) );
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

#define LUMEN_REGISTER_MODULE_DETAIL_IMPL( ModuleType, UniqueId )                                                                  \
    namespace                                                                                                                      \
    {                                                                                                                              \
        [[maybe_unused]] const ::Lumen::Int32 LUMEN_REGISTER_MODULE_DETAIL_CONCAT( GAutoRegisterModule_, UniqueId ) =            \
            [] () noexcept -> ::Lumen::Int32                                                                                     \
        {                                                                                                                        \
            ::Lumen::Launch::RegisterModuleDescriptor(                                                                           \
                ::Lumen::Launch::Internal::TModuleStorage<ModuleType>::Descriptor );                                               \
            return 0;                                                                                                            \
        }(); \
    } // namespace
