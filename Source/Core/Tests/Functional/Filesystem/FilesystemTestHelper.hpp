/**
 * @file FilesystemTestHelper.hpp
 * @brief Shared fixture and path helpers for the filesystem functional tests.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <Filesystem/Path.hpp>

namespace Lumen
{

namespace Tests
{

    /** @brief Root of the scratch area used by the filesystem tests, relative to the test working directory. */
    [[nodiscard]] Filesystem::FPath GetTestRoot () noexcept;

    /** @brief Path of a per-test scratch directory. */
    [[nodiscard]] Filesystem::FPath GetTestPath ( const AnsiChar *InName ) noexcept;

    /**
     * @struct FScopedTestDir
     * @brief Creates a per-test scratch directory and removes it on destruction, along with the shared root
     *        once no test directory remains.
     */
    struct FScopedTestDir
    {
        Filesystem::FPath Path;

        explicit FScopedTestDir ( const AnsiChar *InName, const Bool bCreate = true ) noexcept;
        ~FScopedTestDir () noexcept;
    };

} // namespace Tests

} // namespace Lumen
