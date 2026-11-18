/**
 * @file RHIQueueType.hpp
 * @brief Defines portable queue classes.
 */
#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIQueueType
     * @brief Work classes accepted by a queue.
     */
    enum class ERHIQueueType : UInt8
    {
        Graphics = 0,
        Compute,
        Copy,
        Count
    };

    /**
     * @struct FRHIQueueFamilyInfo
     * @brief Describes one adapter queue family.
     */
    struct FRHIQueueFamilyInfo
    {
        ERHIQueueType Type         = ERHIQueueType::Graphics;
        UInt32 Count               = 0U;
        Bool bSupportsPresentation = false;
        Bool bSupportsTimestamps   = false;
    };

} // namespace RHI

} // namespace Lumen
