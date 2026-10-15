///
/// @file LaunchEngineModule.hpp
/// @brief Contract between the launcher (composition root) and the engine modules.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <Core/EnumFlags.hpp>

#include <Engine/LaunchEngineConfig.hpp>
#include <Launch/LaunchError.hpp>

#include <Memory/Span.hpp>
#include <Memory/UniquePtr.hpp>

#include <String/StringView.hpp>

namespace Lumen
{

namespace Launch
{
    ///
    /// @enum EModuleTraits
    /// @brief Declares which per-frame hooks a module actually implements.
    ///
    enum class EModuleTraits : UInt8
    {
        None      = 0,
        FixedTick = 1 << 0,
        Tick      = 1 << 1,
    };

    LUMEN_ENUM_FLAGS( EModuleTraits )

    ///
    /// @class IEngineModule
    /// @brief A subsystem whose lifetime is bound to its C++ lifetime.
    ///
    class IEngineModule
    {
    public:

        virtual ~IEngineModule () noexcept = default;

    public:

        /// @return The name of the module.
        [[nodiscard]] virtual FStringView GetName () const noexcept = 0;

        /// @return The traits of the module.
        [[nodiscard]] virtual EModuleTraits GetTraits () const noexcept;

        /// @brief Fixed Tick simulation hook (gameplay, physics...).
        /// @param InFixedDeltaSeconds The fixed delta time in seconds.
        virtual void FixedTick ( Float64 InFixedDeltaSeconds ) noexcept;

        /// @brief Tick once per frame.
        /// @param InDeltaSeconds The delta time in seconds.
        /// @param InAlpha The interpolation factor value.
        virtual void Tick ( Float64 InDeltaSeconds, Float64 InAlpha ) noexcept;
    };

    /// @brief Plain module function pointer.
    using FModuleFactory = TLaunchResult<TUniquePtr<IEngineModule>> ( * )( const FLaunchEngineConfig & ) noexcept;

    ///
    /// @struct FModuleDescriptor
    /// @brief Static registration entry. Order in the table is startup order; shutdown is the exact reverse.
    ///
    struct FModuleDescriptor
    {
        FStringView Name;
        FModuleFactory Create;
    };

    /// @brief The registration table. Defined in `EngineModules.cpp`, the only file to edit when adding a module.
    [[nodiscard]] TSpan<const FModuleDescriptor> GetEngineModules () noexcept;

} // namespace Launch

} // namespace Lumen
