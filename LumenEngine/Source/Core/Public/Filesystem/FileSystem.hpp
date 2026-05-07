/**
 * @file FileSystem.hpp
 * @brief Global standard filesystem operations.
 */

#pragma once

#include "Container/Expected.hpp"
#include "Definitions.hpp"
#include "ErrorCodes.hpp"

#include "Filesystem/Path.hpp"
#include "Filesystem/Types.hpp"

namespace LumenEngine
{

/**
 * @class FFileSystem
 * @brief System-wide filesystem utilities.
 */
class LUMEN_ENGINE_API FFileSystem final
{
public:

    /**
     * @brief Queries the OS for disk capacity and available space.
     * @param InPath Any path on the target volume.
     * @return Disk space info or error code.
     */
    [[nodiscard]] static TExpected<Filesystem::FDiskSpaceInfo, EErrorCode::Type> GetDiskSpace ( const Filesystem::FPath &InPath ) noexcept;

    /**
     * @brief Returns the path to the system's temporary directory.
     */
    [[nodiscard]] static Filesystem::FPath GetTempDirectory () noexcept;

    /**
     * @brief Returns the current working directory of the process.
     */
    [[nodiscard]] static TExpected<Filesystem::FPath, EErrorCode::Type> GetCurrentDirectory () noexcept;

    /**
     * @brief Changes the current working directory of the process.
     */
    static TExpected<void, EErrorCode::Type> SetCurrentDirectory ( const Filesystem::FPath &InPath ) noexcept;
};

} // namespace LumenEngine
