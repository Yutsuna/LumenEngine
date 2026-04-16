/**
 * @file GameApplication.inl
 * @brief Implementation of the inline functions for the Game Application API
 */

#pragma once

#include "GameApplication.hpp"

#define LUMEN_REGISTER_GAME_APPLICATION( GameApplicationType )                                                                                                           \
    namespace                                                                                                                                                            \
    {                                                                                                                                                                    \
                                                                                                                                                                         \
    struct FAutoRegisterGameApplication                                                                                                                                  \
    {                                                                                                                                                                    \
        GameApplicationType GInstance;                                                                                                                                   \
                                                                                                                                                                         \
        FAutoRegisterGameApplication ()                                                                                                                                  \
        {                                                                                                                                                                \
            LumenEngine::Launch::RegisterGameApplication( &GInstance );                                                                                                  \
        }                                                                                                                                                                \
                                                                                                                                                                         \
    } GAutoRegisterGameApplication;                                                                                                                                      \
                                                                                                                                                                         \
    } // namespace
