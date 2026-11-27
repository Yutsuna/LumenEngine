/**
 * @file RenderAssetRegistry.cpp
 * @brief Implements control-time geometry upload and sorted asset storage.
 */

#include <Frame/RenderAssetRegistry.hpp>

#include <Algo/Any.hpp>

#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHIUploadAllocator.hpp>
#include <RHI/Memory/RHIMemoryClass.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Queue/RHISubmitDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIResourceState.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

#include <cstring>
#include <utility>

namespace
{

/** @brief Frees a staged upload range on every exit path after allocation. */
class FScopedUploadRange final
{
public:

    FScopedUploadRange ( const Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator> &InAllocator,
                         const Lumen::RHI::FRHIUploadAllocation &InAllocation ) noexcept
        : Allocator( InAllocator ), Allocation( InAllocation )
    {
        /** Empty */
    }

    FScopedUploadRange ( const FScopedUploadRange & )           = delete;
    FScopedUploadRange &operator=( const FScopedUploadRange & ) = delete;
    FScopedUploadRange ( FScopedUploadRange && )                = delete;
    FScopedUploadRange &operator=( FScopedUploadRange && )      = delete;

    ~FScopedUploadRange () noexcept
    {
        Free();
    }

    /** @brief Releases the range once; later calls are no-ops. */
    void Free () noexcept
    {
        if ( bActive )
        {
            static_cast<void>( Allocator->Free( Allocation.Allocation ) );
            bActive = false;
        }
    }

private:

    Lumen::TSharedRef<Lumen::RHI::IRHIUploadAllocator> Allocator;
    Lumen::RHI::FRHIUploadAllocation Allocation;
    Lumen::Bool bActive = true;
};

/** @return Index of the first asset whose id is not less than InId. */
[[nodiscard]] Lumen::USize LowerBound ( const Lumen::TVector<Lumen::FRenderGeometryAsset> &InAssets,
                                        const Lumen::FRenderAssetId InId ) noexcept
{
    Lumen::USize First = 0ULL;
    Lumen::USize Count = InAssets.size();
    while ( First < Count )
    {
        const Lumen::USize Middle = First + ( ( Count - First ) / 2ULL );
        if ( InAssets[Middle].Id < InId )
        {
            First = Middle + 1ULL;
        }
        else
        {
            Count = Middle;
        }
    }
    return First;
}

/** @return True when indices form nonempty triangles that reference existing positions. */
[[nodiscard]] Lumen::Bool IsValidGeometry ( const Lumen::FRenderGeometryAssetDesc &InDesc ) noexcept
{
    if ( not InDesc.Id.IsValid() or InDesc.Positions.size() < 3ULL or InDesc.Indices.size() < 3ULL or
         ( InDesc.Indices.size() % 3ULL ) != 0ULL )
    {
        return false;
    }
    return not Lumen::Algo::AnyOf( InDesc.Indices, [&InDesc] ( const Lumen::UInt32 InIndex ) noexcept -> Lumen::Bool
    {
        return static_cast<Lumen::USize>( InIndex ) >= InDesc.Positions.size();
    } );
}

/** @brief Stages geometry through one copy submission and returns the resulting asset. */
[[nodiscard]] Lumen::TExpected<Lumen::FRenderGeometryAsset, Lumen::RHI::FRHIError>
CreateGeometryAsset ( Lumen::RHI::IRHIDevice &InDevice,
                      Lumen::RHI::IRHIQueue &InQueue,
                      const Lumen::FRenderGeometryAssetDesc &InDesc,
                      const Lumen::UInt64 InVertexBytes,
                      const Lumen::UInt64 InIndexBytes,
                      const Lumen::UInt32 InVertexCount,
                      const Lumen::UInt32 InIndexCount ) noexcept
{
    const Lumen::UInt64 TotalBytes = InVertexBytes + InIndexBytes;
    LUMEN_EXPECT( UploadAllocator, InDevice.CreateUploadAllocator( TotalBytes ) );
    LUMEN_EXPECT( Staging, UploadAllocator->Allocate( Lumen::RHI::FRHIAllocationDesc{
                               .Size = TotalBytes, .Alignment = 16ULL, .MemoryClass = Lumen::RHI::ERHIMemoryClass::Upload } ) );

    FScopedUploadRange UploadRange( UploadAllocator, Staging );
    std::memcpy( Staging.MappedData.data(), InDesc.Positions.data(), static_cast<Lumen::USize>( InVertexBytes ) );
    std::memcpy( Staging.MappedData.data() + static_cast<Lumen::USize>( InVertexBytes ), InDesc.Indices.data(),
                 static_cast<Lumen::USize>( InIndexBytes ) );

    const Lumen::RHI::FRHIBufferDesc VertexDesc{ .Size  = InVertexBytes,
                                                 .Usage = Lumen::RHI::ERHIResourceUsage::VertexBuffer |
                                                          Lumen::RHI::ERHIResourceUsage::CopySource |
                                                          Lumen::RHI::ERHIResourceUsage::CopyDestination,
                                                 .MemoryClass  = Lumen::RHI::ERHIMemoryClass::DeviceLocal,
                                                 .InitialState = Lumen::RHI::ERHIResourceState::Common };
    const Lumen::RHI::FRHIBufferDesc IndexDesc{ .Size  = InIndexBytes,
                                                .Usage = Lumen::RHI::ERHIResourceUsage::IndexBuffer |
                                                         Lumen::RHI::ERHIResourceUsage::CopySource |
                                                         Lumen::RHI::ERHIResourceUsage::CopyDestination,
                                                .MemoryClass  = Lumen::RHI::ERHIMemoryClass::DeviceLocal,
                                                .InitialState = Lumen::RHI::ERHIResourceState::Common };

    LUMEN_EXPECT( VertexBuffer, InDevice.CreateBuffer( VertexDesc ) );
    LUMEN_EXPECT( IndexBuffer, InDevice.CreateBuffer( IndexDesc ) );
    LUMEN_EXPECT( CommandAllocator, InDevice.CreateCommandAllocator( InQueue.GetType() ) );
    LUMEN_EXPECT( CommandList, InDevice.CreateCommandList( CommandAllocator ) );

    LUMEN_TRY_EXPECT_ERROR( CommandList->Begin() );
    LUMEN_TRY_EXPECT_ERROR(
        CommandList->CopyBuffer( Staging.Buffer, VertexBuffer, Lumen::RHI::FRHIBufferCopy{ .Size = InVertexBytes } ) );
    LUMEN_TRY_EXPECT_ERROR( CommandList->CopyBuffer(
        Staging.Buffer, IndexBuffer, Lumen::RHI::FRHIBufferCopy{ .SourceOffset = InVertexBytes, .Size = InIndexBytes } ) );
    LUMEN_TRY_EXPECT_ERROR( CommandList->End() );

    const Lumen::UInt64 LastCompletedValue = InQueue.GetCompletedValue().Value;
    if ( LastCompletedValue == ~0ULL )
    {
        return Lumen::TUnexpected<Lumen::RHI::FRHIError>(
            Lumen::RHI::MakeRHIInvalidQueue( "Queue completion value cannot advance" ) );
    }

    Lumen::RHI::FRHISubmitDesc Submit;
    Submit.CommandLists.push_back( CommandList );
    Submit.Completion = { LastCompletedValue + 1ULL };
    LUMEN_TRY_EXPECT_ERROR( InQueue.Submit( Submit ) );
    LUMEN_TRY_EXPECT_ERROR( InQueue.Wait( Submit.Completion, 1.0 ) );
    LUMEN_TRY_EXPECT_ERROR( CommandAllocator->Reset( InQueue.GetCompletedValue() ) );
    UploadRange.Free();

    return Lumen::FRenderGeometryAsset{ .Id           = InDesc.Id,
                                        .VertexBuffer = VertexBuffer,
                                        .IndexBuffer  = IndexBuffer,
                                        .VertexCount  = InVertexCount,
                                        .IndexCount   = InIndexCount,
                                        .IndexFormat  = Lumen::RHI::ERHIIndexFormat::Index32 };
}

} // namespace

Lumen::FRenderAssetRegistry::FRenderAssetRegistry ( const USize InCapacity ) noexcept : Capacity( InCapacity )
{
    Assets.reserve( InCapacity );
}

Lumen::TExpected<void, Lumen::ERenderAssetError> Lumen::FRenderAssetRegistry::Register (
    RHI::IRHIDevice &InDevice, RHI::IRHIQueue &InQueue, const FRenderGeometryAssetDesc &InDesc ) noexcept
{
    const TOptional<UInt32> VertexCount = RHI::RHICheckedCast<UInt32>( InDesc.Positions.size() );
    const TOptional<UInt32> IndexCount  = RHI::RHICheckedCast<UInt32>( InDesc.Indices.size() );
    if ( not VertexCount or not IndexCount or not IsValidGeometry( InDesc ) )
    {
        return TUnexpected<ERenderAssetError>( ERenderAssetError::InvalidDesc );
    }

    const TOptional<UInt64> PositionBytes =
        RHI::RHICheckedMultiply( static_cast<UInt64>( *VertexCount ), static_cast<UInt64>( sizeof( Maths::FVec3f ) ) );
    const TOptional<UInt64> IndexBytes =
        RHI::RHICheckedMultiply( static_cast<UInt64>( *IndexCount ), static_cast<UInt64>( sizeof( UInt32 ) ) );
    if ( not PositionBytes or not IndexBytes )
    {
        return TUnexpected<ERenderAssetError>( ERenderAssetError::InvalidDesc );
    }
    if ( not RHI::RHICheckedAdd( *PositionBytes, *IndexBytes ) )
    {
        return TUnexpected<ERenderAssetError>( ERenderAssetError::InvalidDesc );
    }

    const USize DuplicateIndex = LowerBound( Assets, InDesc.Id );
    if ( DuplicateIndex < Assets.size() and Assets[DuplicateIndex].Id == InDesc.Id )
    {
        return TUnexpected<ERenderAssetError>( ERenderAssetError::InvalidDesc );
    }
    if ( Assets.size() >= Capacity )
    {
        return TUnexpected<ERenderAssetError>( ERenderAssetError::CapacityExceeded );
    }

    auto Created = CreateGeometryAsset( InDevice, InQueue, InDesc, *PositionBytes, *IndexBytes, *VertexCount, *IndexCount );
    if ( not Created ) [[unlikely]]
    {
        return TUnexpected<ERenderAssetError>( ERenderAssetError::RHIError );
    }

    FRenderGeometryAsset Asset = std::move( *Created );
    Assets.insert( Assets.begin() + static_cast<ISize>( DuplicateIndex ), std::move( Asset ) );
    return {};
}

const Lumen::FRenderGeometryAsset *Lumen::FRenderAssetRegistry::Find ( const FRenderAssetId InId ) const noexcept
{
    const USize Index = LowerBound( Assets, InId );
    if ( Index >= Assets.size() or Assets[Index].Id != InId )
    {
        return nullptr;
    }
    return &Assets[Index];
}

Lumen::TSpan<const Lumen::FRenderGeometryAsset> Lumen::FRenderAssetRegistry::GetAssets () const noexcept
{
    return TSpan<const FRenderGeometryAsset>( Assets.data(), Assets.size() );
}

Lumen::USize Lumen::FRenderAssetRegistry::GetSize () const noexcept
{
    return Assets.size();
}

Lumen::USize Lumen::FRenderAssetRegistry::GetCapacity () const noexcept
{
    return Capacity;
}
