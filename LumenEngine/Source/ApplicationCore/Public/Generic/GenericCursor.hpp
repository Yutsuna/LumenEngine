/**
 * @file GenericCursor.hpp
 * @brief Declaration of the FGenericCursor interface for cursor management.
 */

#pragma once

#include "Definitions.hpp"
#include "Maths/Vector.hpp"

namespace LumenEngine
{

/**
 * @enum ECursorType
 * @brief Enumeration for different types of cursors.
 */
namespace ECursorType
{
    enum Type : Int32
    {
        /** No cursor */
        None,
        /** Default cursor (arrow) */
        Default,
        /** Number of cursors available */
        TotalCount
    };

    /**
     * @brief Convert a cursor type enum value to its string representation.
     */
    static inline const AnsiChar *const ToString ( const Type InCursorType )
    {
        switch ( InCursorType )
        {
        case None:
            return "None";
        case Default:
            return "Default";
        default:
            return "Unknown";
        }
    }

} // namespace ECursorType

/**
 * @class GenericCursor
 * @brief Interface for cursor management in the application.
 */
class LUMEN_ENGINE_API FGenericCursor
{
public:

    virtual ~FGenericCursor () = default;

    /**
     * Sets the position of the cursor.
     *
     * @param Position The new position of the cursor as a 2D vector.
     */
    virtual void SetPosition ( const Math::FVec2d &Position );

    /**
     * Gets the current position of the cursor.
     *
     * @return The current position of the cursor as a 2D vector.
     */
    virtual const Math::FVec2d &GetPosition () const;

    /**
     * Sets the type of the cursor.
     *
     * @param InCursorType The new cursor type.
     */
    virtual void SetCursorType ( const ECursorType::Type InCursorType );

    /**
     * Gets the current type of the cursor.
     *
     * @return The current cursor type.
     */
    virtual ECursorType::Type GetCursorType () const;

    /**
     * Shows or hides the cursor
     *
     * @param bShow True to show the cursor, false to hide it
     */
    virtual void ShowCursor ( const Bool bShow ) = 0;

    /**
     * Locks the cursor to the passed in bounds
     *
     * @param InBounds The bounds to lock the cursor to
     */
    virtual void LockCursorToBounds ( const Math::FVec4d &InBounds ) = 0;

    /**
     * Gets whether the cursor is currently visible
     *
     * @return true if the cursor is visible, false otherwise
     */
    virtual Bool IsCursorVisible () const;

protected:

    Math::FVec2d CurrentPosition;
    ECursorType::Type CurrentCursorType;

    bool bHidden;
};

} // namespace LumenEngine
