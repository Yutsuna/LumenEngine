/**
 * @file File.inl
 * @brief Inline implementations for file handling utilities in Lumen Engine.
 */

#pragma once

namespace LumenEngine
{

namespace Filesystem
{

    template <typename Type>
    TExpected<TVector<Type>, EErrorCode::Type> FFile::ReadAllBytes ( const FPath &InPath ) noexcept
    {
        auto FileResult = Open( InPath, EFileMode::Read );
        if ( not FileResult )
        {
            return MakeUnexpected( FileResult.error() );
        }

        auto InfoResult = GetInfo( InPath );
        if ( not InfoResult )
        {
            return MakeUnexpected( InfoResult.error() );
        }

        const USize SizeInBytes = InfoResult.value().SizeBytes;
        if ( SizeInBytes == 0 )
        {
            return TVector<Type>();
        }

        if ( SizeInBytes % sizeof( Type ) != 0 )
        {
            return MakeUnexpected( EErrorCode::Failure );
        }

        TVector<Type> Content( SizeInBytes / sizeof( Type ) );

        auto ReadResult = FileResult.value()->Read( Content.data(), SizeInBytes );
        if ( not ReadResult )
        {
            return MakeUnexpected( ReadResult.error() );
        }

        return Content;
    }

    template <typename Type>
    TExpected<void, EErrorCode::Type> FFile::WriteAllBytes ( const FPath &InPath, const TVector<Type> &InData ) noexcept
    {
        auto FileResult = Open( InPath, EFileMode::Write );
        if ( not FileResult )
        {
            return MakeUnexpected( FileResult.error() );
        }

        auto WriteResult = FileResult.value()->Write( InData.data(), InData.size() * sizeof( Type ) );
        if ( not WriteResult )
        {
            return MakeUnexpected( WriteResult.error() );
        }

        return {};
    }

} // namespace Filesystem

} // namespace LumenEngine
