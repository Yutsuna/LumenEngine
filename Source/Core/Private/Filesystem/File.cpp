///
/// @file File.cpp
/// @brief FFile helper implementations.
///

#include <Filesystem/File.hpp>
#include <Filesystem/IFileSystem.hpp>

#include <limits>

namespace Lumen
{

Filesystem::IFileSystem &GetDefaultFileSystem() noexcept;

Bool Filesystem::FFile::Exists(const FPath &InPath) noexcept
{
    return GetDefaultFileSystem().FileExists(InPath);
}

TExpected<FString, Filesystem::EFileError> Filesystem::FFile::ReadAllText(const FPath &InPath)
{
    return ReadAllBytes(InPath).Transform(
        [](const TVector<Byte> &Bytes) -> FString
        {
            return FString(reinterpret_cast<const AnsiChar *>(Bytes.data()), Bytes.size());
        });
}

TExpected<TVector<Byte>, Filesystem::EFileError> Filesystem::FFile::ReadAllBytes(const FPath &InPath)
{
    return GetDefaultFileSystem()
           .OpenFile(InPath, EFileAccess::Read, EFileMode::OpenExisting)
           .AndThen([](const TUniquePtr<IFileHandle> &Handle) -> TExpected<TVector<Byte>, EFileError>
           {
               const UInt64 FileSize = Handle->Size();

               if (FileSize > static_cast<UInt64>(std::numeric_limits<USize>::max()))
               {
                   return MakeUnexpected(EFileError::Unknown);
               }

               TVector<Byte> Buffer;
               Buffer.resize(static_cast<USize>(FileSize));

               USize TotalRead = 0;
               while (TotalRead < Buffer.size())
               {
                   const USize ReadCount = Handle->Read(TSpan<Byte>(Buffer.data() + TotalRead, Buffer.size() - TotalRead));
                   if (ReadCount == 0)
                   {
                       break;
                   }

                   TotalRead += ReadCount;
               }

               Buffer.resize(TotalRead);
               return Buffer;
           });
}

TExpected<void, Filesystem::EFileError> Filesystem::FFile::WriteAllText(const FPath &InPath, FStringView InContent) noexcept
{
    return WriteAllBytes(InPath, TSpan<const Byte>(reinterpret_cast<const Byte *>(InContent.data()), InContent.size()));
}

TExpected<void, Filesystem::EFileError> Filesystem::FFile::WriteAllBytes(const FPath &InPath, TSpan<const Byte> InData) noexcept
{
    return GetDefaultFileSystem()
           .OpenFile(InPath, EFileAccess::Write, EFileMode::CreateAlways)
           .AndThen([&](const TUniquePtr<IFileHandle> &Handle) -> TExpected<void, EFileError>
           {
               USize TotalWritten = 0;

               while (TotalWritten < InData.size())
               {
                   const USize Written = Handle->Write(InData.subspan(TotalWritten));
                   if (Written == 0)
                   {
                       return MakeUnexpected(EFileError::Unknown);
                   }

                   TotalWritten += Written;
               }

               if (not Handle->Flush())
               {
                   return MakeUnexpected(EFileError::Unknown);
               }

               return {};
           });
}

Bool Filesystem::FFile::Delete(const FPath &InPath) noexcept
{
    return GetDefaultFileSystem().DeleteFile(InPath);
}

Bool Filesystem::FFile::Copy(const FPath &InSource, const FPath &InDestination, const Bool bOverwrite) noexcept
{
    return GetDefaultFileSystem().CopyFile(InSource, InDestination, bOverwrite);
}

Bool Filesystem::FFile::Move(const FPath &InSource, const FPath &InDestination, const Bool bOverwrite) noexcept
{
    return GetDefaultFileSystem().MoveFile(InSource, InDestination, bOverwrite);
}

TExpected<UInt64, Filesystem::EFileError> Filesystem::FFile::GetSize(const FPath &InPath) noexcept
{
    return GetDefaultFileSystem().GetStat(InPath).Transform(
        [](const FFileStat &Stat) noexcept -> UInt64
        {
            return Stat.FileSize;
        });
}

} // namespace Lumen