/**
 * @file RHIMemoryBudget.cpp
 * @brief Implements atomic memory budget accounting.
 */
#include <Memory/RHIMemoryBudget.hpp>
Lumen::FRHIMemoryBudget::FRHIMemoryBudget ( const UInt64 InCapacity ) noexcept : Capacity( InCapacity ), Used( 0ULL )
{
}
Lumen::Bool Lumen::FRHIMemoryBudget::TryReserve ( const UInt64 InSize ) noexcept
{
    UInt64 Current = Used.load( std::memory_order_relaxed );
    while ( true )
    {
        if ( InSize > Capacity or Current > Capacity - InSize )
        {
            return false;
        }
        if ( Used.compare_exchange_weak( Current, Current + InSize, std::memory_order_acq_rel ) )
        {
            return true;
        }
    }
}
void Lumen::FRHIMemoryBudget::Release ( const UInt64 InSize ) noexcept
{
    UInt64 Current = Used.load( std::memory_order_relaxed );
    while ( not Used.compare_exchange_weak( Current, InSize <= Current ? Current - InSize : 0ULL, std::memory_order_acq_rel ) )
    {
    }
}
Lumen::UInt64 Lumen::FRHIMemoryBudget::GetUsed () const noexcept
{
    return Used.load( std::memory_order_acquire );
}
