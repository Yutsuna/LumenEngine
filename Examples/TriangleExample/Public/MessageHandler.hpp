/**
 * @file MessageHandler.hpp
 * @brief Message handler for the base example application.
 */

#pragma once

#include "Generic/GenericApplicationMessageHandler.hpp"

namespace LumenEngine
{

class FTriangleExampleMessageHandler final : public FGenericApplicationMessageHandler
{
public:

    void OnRequestExit () override;
};

} // namespace LumenEngine
