/**
 * @file MessageHandler.hpp
 * @brief Message handler for the base example application.
 */

#pragma once

#include "Generic/GenericApplicationMessageHandler.hpp"

namespace LumenEngine
{

class FBaseExampleMessageHandler final : public FGenericApplicationMessageHandler
{
public:

    void OnWindowCloseRequested ( const TSharedRef<FGenericWindow> &InWindow ) override;
    void OnRequestExit () override;
};

} // namespace LumenEngine
