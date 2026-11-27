/**
 * @file SceneHierarchy.inl
 * @brief Allocation-free visitor for level-ordered hierarchy iteration.
 */

#pragma once

#include <Hierarchy/SceneHierarchy.hpp>

template <typename TVisitor>
void Lumen::FSceneHierarchy::ForEachLevel ( const USize InDepth, TVisitor &&InVisitor ) const noexcept
{
    if ( not LevelStarts.IsValid() or InDepth >= Capacity )
    {
        return;
    }

    const USize Begin = LevelStarts[InDepth];
    const USize End   = LevelStarts[InDepth + 1ULL];
    for ( USize Index = Begin; Index < End; ++Index )
    {
        InVisitor( LevelOrderedTransforms[Index] );
    }
}
