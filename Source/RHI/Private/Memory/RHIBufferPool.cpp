/**
 * @file RHIBufferPool.cpp
 * @brief Implements device-backed buffer acquisition and safe retention.
 */
#include "RHIBufferPool.hpp"
#include <Resources/IRHIBuffer.hpp>
#include <Templates/Hash.hpp>
Lumen::FRHIBufferPool::FRHIBufferPool ( IRHIDevice &InDevice, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIBufferPool>() ), Device( &InDevice )
{
}
LUMEN_RHI_OBJECT_FORWARDERS( FRHIBufferPool )
Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIBuffer>, Lumen::FRHIError>
Lumen::FRHIBufferPool::Acquire ( const FRHIBufferDesc &InDesc ) noexcept
{
    if ( const TExpected<void, FRHIError> Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    for ( USize Index = 0U; Index < Released.size(); ++Index )
    {
        if ( not Released[Index].Completion.IsValid() and Released[Index].Buffer->GetDesc().Size == InDesc.Size and
             Released[Index].Buffer->GetDesc().Usage == InDesc.Usage )
        {
            TSharedRef<IRHIBuffer> Result = Released[Index].Buffer;
            Released.erase( Released.begin() + static_cast<ISize>( Index ) );
            return Result;
        }
    }
    return Device->CreateBuffer( InDesc );
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferPool::Release ( TSharedRef<IRHIBuffer> InBuffer,
                                                                          const FRHITimelineValue InCompletion ) noexcept
{
    Released.push_back( FEntry{ std::move( InBuffer ), InCompletion } );
    return {};
}
