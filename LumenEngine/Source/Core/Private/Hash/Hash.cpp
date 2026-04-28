/**
 * @file Hash.cpp
 * @brief Implementation of Hash functions and functors in LumenEngine
 */

#include "Hash/Hash.hpp"

namespace
{

constexpr LumenEngine::FHashValue WangMix64 ( LumenEngine::FHashValue InKey ) noexcept
{
    InKey = ( ~InKey ) + ( InKey << 21U );
    InKey = InKey ^ ( InKey >> 24U );
    InKey = ( InKey + ( InKey << 3U ) ) + ( InKey << 8U );
    InKey = InKey ^ ( InKey >> 14U );
    InKey = ( InKey + ( InKey << 2U ) ) + ( InKey << 4U );
    InKey = InKey ^ ( InKey >> 28U );
    InKey = InKey + ( InKey << 31U );
    return InKey;
}

template <typename TInt> struct TIntHasher final
{
    [[nodiscard]] LumenEngine::FHashValue operator()( const TInt &InValue ) const noexcept
    {
        return WangMix64( static_cast<LumenEngine::FHashValue>( InValue ) );
    }
};

} // namespace
