/**
 * @file Thread.hpp
 * @brief Move-only thread with cooperative cancellation and automatic joining.
 */

#pragma once

#include <CoreExport.hpp>

#include <Core/NonCopyable.hpp>

#include <Concepts/IsInvocable.hpp>
#include <Concepts/SameAs.hpp>

#include <Templates/StopToken.hpp>

#include <HAL/PlatformThread.hpp>

namespace Lumen
{

/**
 * @class FThread
 * @brief Move-only thread with cooperative cancellation and automatic joining.
 * @tparam TFunction The callable type to invoke on the thread.
 * @tparam TArgs The types of the arguments to pass to the callable.
 */
class CORE_EXPORT FThread final : public FNonCopyable
{
public:

    using FId           = HAL::FPlatformThread::FId;
    using FNativeHandle = HAL::FPlatformThread::FNativeHandle;

    /** @brief Creates an empty thread with no stop state. */
    FThread () noexcept = default;
    FThread ( FThread &&InOther ) noexcept;

    /**
     * @brief Starts a thread with a callable and decayed arguments.
     * @details A callable accepting FStopToken as its first argument receives this thread's stop token. Other callables
     *          are invoked with only the provided arguments. If creation fails, the thread is empty and has no stop state.
     */
    template <typename TFunction, typename... TArgs>
        requires( not Concepts::CSameAs<std::remove_cvref_t<TFunction>, FThread> ) &&
                ( Concepts::CIsInvocable<std::decay_t<TFunction>, FStopToken, std::decay_t<TArgs>...> ||
                  Concepts::CIsInvocable<std::decay_t<TFunction>, std::decay_t<TArgs>...> )
    explicit FThread( TFunction &&InFunction, TArgs &&...InArgs ) noexcept;

    /** @brief Requests stop and joins a joinable thread. */
    ~FThread () noexcept;

    /** @brief Requests stop and joins the current thread before taking ownership of InOther. */
    FThread &operator=( FThread &&InOther ) noexcept;

    void Join () noexcept;
    void Detach () noexcept;
    void Swap ( FThread &InOther ) noexcept;

    [[nodiscard]] Bool IsJoinable () const noexcept;
    [[nodiscard]] Bool RequestStop () noexcept;
    [[nodiscard]] FStopSource GetStopSource () const noexcept;
    [[nodiscard]] FStopToken GetStopToken () const noexcept;
    [[nodiscard]] FId GetId () const noexcept;
    [[nodiscard]] FNativeHandle GetNativeHandle () const noexcept;

    [[nodiscard]] static FId GetCurrentId () noexcept;
    [[nodiscard]] static UInt32 HardwareConcurrency () noexcept;
    static void Yield () noexcept;

private:

    FStopSource StopSource{ nullptr };
    HAL::FPlatformThread Thread;
};

} // namespace Lumen

#include "Inline/Thread.inl"
