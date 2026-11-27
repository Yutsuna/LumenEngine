/**
 * @file MessageHandler.hpp
 * @brief Handles incoming messages from the Engine.
 */

#pragma once

#include <Generic/GenericApplicationMessageHandler.hpp>

#include <Memory/SharedPtr.hpp>

namespace Lumen
{

/**
 * @class FLuminaMessageHandler
 * @brief Decorates the engine message handler with game-level behaviour.
 * @note Pure adapter: it depends on the engine handler contract, never on the module that installs it.
 */
class FLuminaMessageHandler final : public FGenericApplicationMessageHandler
{
public:

    explicit FLuminaMessageHandler ( TSharedPtr<FGenericApplicationMessageHandler> InNextHandler ) noexcept;

public:

    void OnRequestExit () noexcept override;
    void OnQuit () noexcept override;

private:

    TSharedPtr<FGenericApplicationMessageHandler> NextHandler;
};

} // namespace Lumen
