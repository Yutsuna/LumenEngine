/**
 * @file IRHIAllocationObserver.hpp
 * @brief Declares allocation lifecycle notifications and their shared registry.
 */
#pragma once
#include <Core/CoreTypes.hpp>
#include <Memory/RHIAllocation.hpp>
#include <Templates/Vector.hpp>
namespace Lumen
{

/**
 * @class IRHIAllocationObserver
 * @brief Observer used by accounting, diagnostics, and defragmentation.
 */
class IRHIAllocationObserver
{
public:

    virtual ~IRHIAllocationObserver () noexcept = default;
    /** @brief Called after a successful allocation. */
    virtual void OnAllocationCreated ( const FRHIAllocation &InAllocation ) noexcept = 0;
    /** @brief Called after a successful release. */
    virtual void OnAllocationReleased ( const FRHIAllocation &InAllocation ) noexcept = 0;
    /** @brief Called after a defragmentation move has been executed. */
    virtual void OnAllocationMoved ( const FRHIAllocation &InBefore, const FRHIAllocation &InAfter ) noexcept = 0;
};

/**
 * @class FRHIAllocationObserverRegistry
 * @brief Ordered, non-owning collection of allocation lifecycle observers.
 * @details The memory facade registers observers here, and both the facade and the
 *          defragmenter deliver notifications through this registry. Observers must
 *          outlive their registration; notifications arrive in registration order.
 *          The registry is not internally synchronized.
 */
class FRHIAllocationObserverRegistry final
{
public:

    /** @brief Adds an observer unless it is already registered. */
    void Register ( IRHIAllocationObserver &InObserver ) noexcept
    {
        for ( IRHIAllocationObserver *Observer : Observers )
        {
            if ( Observer == &InObserver )
            {
                return;
            }
        }
        Observers.push_back( &InObserver );
    }

    /** @brief Removes an observer; unknown observers are ignored. */
    void Unregister ( IRHIAllocationObserver &InObserver ) noexcept
    {
        for ( USize Index = 0U; Index < Observers.size(); ++Index )
        {
            if ( Observers[Index] == &InObserver )
            {
                Observers.erase( Observers.begin() + static_cast<ISize>( Index ) );
                return;
            }
        }
    }

    /** @brief Delivers a creation notification to every registered observer. */
    void NotifyCreated ( const FRHIAllocation &InAllocation ) noexcept
    {
        for ( IRHIAllocationObserver *Observer : Observers )
        {
            Observer->OnAllocationCreated( InAllocation );
        }
    }

    /** @brief Delivers a release notification to every registered observer. */
    void NotifyReleased ( const FRHIAllocation &InAllocation ) noexcept
    {
        for ( IRHIAllocationObserver *Observer : Observers )
        {
            Observer->OnAllocationReleased( InAllocation );
        }
    }

    /** @brief Delivers a move notification to every registered observer. */
    void NotifyMoved ( const FRHIAllocation &InBefore, const FRHIAllocation &InAfter ) noexcept
    {
        for ( IRHIAllocationObserver *Observer : Observers )
        {
            Observer->OnAllocationMoved( InBefore, InAfter );
        }
    }

    /** @return Number of currently registered observers. */
    [[nodiscard]] USize GetCount () const noexcept
    {
        return Observers.size();
    }

    /** @brief Drops every registration; intended for shutdown and test isolation. */
    void Clear () noexcept
    {
        Observers.clear();
    }

private:

    TVector<IRHIAllocationObserver *> Observers;
};

/**
 * @return Registry shared by the allocation facade and the defragmenter.
 */
[[nodiscard]] inline FRHIAllocationObserverRegistry &GetRHIAllocationObserverRegistry () noexcept
{
    static FRHIAllocationObserverRegistry Registry;
    return Registry;
}
} // namespace Lumen
