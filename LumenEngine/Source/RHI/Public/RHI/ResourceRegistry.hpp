/**
 * @file ResourceRegistry.hpp
 * @brief Generational registry for RHI resources.
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"
#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class TResourceRegistry
     * @brief Manages generational handles to prevent use-after-free bugs.
     */
    template <typename ResourceType, typename Tag> class TResourceRegistry
    {
    public:

        using HandleType = TRenderResourceHandle<Tag>;

        TResourceRegistry ()  = default;
        ~TResourceRegistry () = default;

    public:

        /** @brief Transfers ownership of the resource and returns a strictly typed handle */
        HandleType Insert ( ResourceType &&InResource );
        HandleType Insert ( const ResourceType &InResource );

        /** @brief Marks the resource slot as free and increments its generation */
        void Remove ( HandleType InHandle );

        /** @brief Retrieves the resource pointer safely, returns nullptr if validation fails */
        [[nodiscard]] ResourceType *Get ( HandleType InHandle );
        [[nodiscard]] const ResourceType *Get ( HandleType InHandle ) const;

        /** @brief Strictly validates the handle against bounds and its specific generation */
        [[nodiscard]] Bool IsValid ( HandleType InHandle ) const noexcept;

        /** @brief Clears the registry completely */
        void Clear ();

        /** @brief Iterates strictly over active resources */
        template <typename Func> void ForEach ( Func InFunc );

    private:

        /** @brief Internal helper to find or create a slot index. */
        [[nodiscard]] UInt16 AllocateSlot ();

        struct FSlot
        {
            ResourceType Data;
            UInt16 Generation = 0;
            Bool bIsActive    = false;
        };

        TVector<FSlot> Slots;
        TVector<UInt16> FreeIndices;
    };

} // namespace RHI

} // namespace LumenEngine

#include "Inline/ResourceRegistry.inl"
