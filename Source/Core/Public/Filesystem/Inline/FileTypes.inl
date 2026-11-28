/**
 * @file FileTypes.inl
 * @brief Inline implementations for FileTypes.
 */

#pragma once

#include <Filesystem/FileTypes.hpp>

/**
 * Public
 */

constexpr Lumen::Bool Lumen::Filesystem::FFileStat::IsValid () const noexcept
{
    return bExists;
}
