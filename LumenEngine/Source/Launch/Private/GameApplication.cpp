/**
 * @file GameApplication.cpp
 * @brief Implementation of the Game Application API
 */

#include "GameApplication.hpp"

namespace
{

LumenEngine::IGameApplication *GGameApplicationInstance = nullptr;

}

void LumenEngine::Launch::RegisterGameApplication ( IGameApplication *InGameApplicationInstance ) noexcept
{
    if ( GGameApplicationInstance == nullptr )
    {
        GGameApplicationInstance = InGameApplicationInstance;
    }
}

LumenEngine::IGameApplication &LumenEngine::Launch::GetGameApplication () noexcept
{
    return *GGameApplicationInstance;
}
