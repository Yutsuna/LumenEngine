/**
 * @file MemoryMappedFileTest.cpp
 * @brief Unit tests for the FMemoryMappedFile class.
 */

#include "Filesystem/MemoryMappedFile.hpp"
#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"
#include <gtest/gtest.h>

namespace
{

class FMemoryMappedFileTest : public ::testing::Test
{
protected:

    void SetUp () override
    {
        TestFilePath = "MemoryMappedFileTest_Temp.bin";
        TestData     = { 0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78 };

        if ( LumenEngine::Filesystem::FFile::Exists( TestFilePath ) )
        {
            if ( not LumenEngine::Filesystem::FFile::Delete( TestFilePath ) )
            {
                FAIL() << "Failed to delete existing test file: " << TestFilePath.ToString();
            }
        }
    }

    void TearDown () override
    {
        if ( LumenEngine::Filesystem::FFile::Exists( TestFilePath ) )
        {
            if ( not LumenEngine::Filesystem::FFile::Delete( TestFilePath ) )
            {
                FAIL() << "Failed to delete test file: " << TestFilePath.ToString();
            }
        }
    }

    LumenEngine::Filesystem::FPath TestFilePath;
    LumenEngine::TVector<LumenEngine::Byte> TestData;
};

} // namespace

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_MissingFile )
{
    auto Result = LumenEngine::Filesystem::FMemoryMappedFile::Open( "NonExistentFile.bin" );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::EErrorCode::NotFound );
}

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_BasicRead )
{
    auto WriteResult = LumenEngine::Filesystem::FFile::WriteAllBytes( TestFilePath, TestData );
    ASSERT_TRUE( WriteResult.has_value() );

    auto MapResult = LumenEngine::Filesystem::FMemoryMappedFile::Open( TestFilePath );
    ASSERT_TRUE( MapResult.has_value() );

    LumenEngine::TUniquePtr<LumenEngine::Filesystem::FMemoryMappedFile> MappedFile = std::move( MapResult.value() );
    ASSERT_TRUE( MappedFile.IsValid() );
    EXPECT_TRUE( MappedFile->IsMapped() );
    EXPECT_EQ( MappedFile->GetSize(), TestData.size() );

    LumenEngine::TSpan<const LumenEngine::Byte> Region = MappedFile->GetRegion();
    EXPECT_EQ( Region.size(), TestData.size() );
    for ( LumenEngine::USize I = 0; I < TestData.size(); ++I )
    {
        EXPECT_EQ( Region[I], TestData[I] );
    }

    MappedFile->Close();
    EXPECT_FALSE( MappedFile->IsMapped() );
    EXPECT_EQ( MappedFile->GetSize(), 0U );
    EXPECT_TRUE( MappedFile->GetRegion().empty() );
}

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_MoveSemantics )
{
    auto WriteResult = LumenEngine::Filesystem::FFile::WriteAllBytes( TestFilePath, TestData );
    ASSERT_TRUE( WriteResult.has_value() );

    auto MapResult = LumenEngine::Filesystem::FMemoryMappedFile::Open( TestFilePath );
    ASSERT_TRUE( MapResult.has_value() );

    LumenEngine::TUniquePtr<LumenEngine::Filesystem::FMemoryMappedFile> MappedFile1 = std::move( MapResult.value() );

    LumenEngine::Filesystem::FMemoryMappedFile MappedFile2( std::move( *MappedFile1 ) );
    EXPECT_FALSE( MappedFile1->IsMapped() );
    EXPECT_TRUE( MappedFile2.IsMapped() );
    EXPECT_EQ( MappedFile2.GetSize(), TestData.size() );

    LumenEngine::Filesystem::FMemoryMappedFile MappedFile3;
    MappedFile3 = std::move( MappedFile2 );
    EXPECT_FALSE( MappedFile2.IsMapped() );
    EXPECT_TRUE( MappedFile3.IsMapped() );
    EXPECT_EQ( MappedFile3.GetSize(), TestData.size() );
}

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_EmptyFile )
{
    LumenEngine::TVector<LumenEngine::Byte> EmptyData;
    auto WriteResult = LumenEngine::Filesystem::FFile::WriteAllBytes( TestFilePath, EmptyData );
    ASSERT_TRUE( WriteResult.has_value() );

    auto MapResult = LumenEngine::Filesystem::FMemoryMappedFile::Open( TestFilePath );
    EXPECT_FALSE( MapResult.has_value() );
    EXPECT_EQ( MapResult.error(), LumenEngine::EErrorCode::InvalidArgument );
}

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_Directory )
{
    LumenEngine::Filesystem::FPath DirPath = "MemoryMappedFileTest_Dir";
    if ( not LumenEngine::Filesystem::FDirectory::Exists( DirPath ) )
    {
        auto CreateResult = LumenEngine::Filesystem::FDirectory::Create( DirPath );
        ASSERT_TRUE( CreateResult.has_value() );
    }

    auto MapResult = LumenEngine::Filesystem::FMemoryMappedFile::Open( DirPath );
    EXPECT_FALSE( MapResult.has_value() );
    EXPECT_EQ( MapResult.error(), LumenEngine::EErrorCode::InvalidArgument );

    auto DeleteResult = LumenEngine::Filesystem::FDirectory::Delete( DirPath );
    EXPECT_TRUE( DeleteResult.has_value() );
}

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_DoubleClose )
{
    auto WriteResult = LumenEngine::Filesystem::FFile::WriteAllBytes( TestFilePath, TestData );
    ASSERT_TRUE( WriteResult.has_value() );

    auto MapResult = LumenEngine::Filesystem::FMemoryMappedFile::Open( TestFilePath );
    ASSERT_TRUE( MapResult.has_value() );

    LumenEngine::TUniquePtr<LumenEngine::Filesystem::FMemoryMappedFile> MappedFile = std::move( MapResult.value() );
    EXPECT_TRUE( MappedFile->IsMapped() );

    MappedFile->Close();
    EXPECT_FALSE( MappedFile->IsMapped() );

    MappedFile->Close();
    EXPECT_FALSE( MappedFile->IsMapped() );
}

TEST_F( FMemoryMappedFileTest, Test_FMemoryMappedFile_MoveAssignmentOverActive )
{
    auto WriteResult = LumenEngine::Filesystem::FFile::WriteAllBytes( TestFilePath, TestData );
    ASSERT_TRUE( WriteResult.has_value() );

    auto MapResult1 = LumenEngine::Filesystem::FMemoryMappedFile::Open( TestFilePath );
    auto MapResult2 = LumenEngine::Filesystem::FMemoryMappedFile::Open( TestFilePath );

    ASSERT_TRUE( MapResult1.has_value() );
    ASSERT_TRUE( MapResult2.has_value() );

    LumenEngine::Filesystem::FMemoryMappedFile MappedFile1 = std::move( *MapResult1.value() );
    LumenEngine::Filesystem::FMemoryMappedFile MappedFile2 = std::move( *MapResult2.value() );

    EXPECT_TRUE( MappedFile1.IsMapped() );
    EXPECT_TRUE( MappedFile2.IsMapped() );

    MappedFile1 = std::move( MappedFile2 );

    EXPECT_TRUE( MappedFile1.IsMapped() );
    EXPECT_FALSE( MappedFile2.IsMapped() );
    EXPECT_EQ( MappedFile1.GetSize(), TestData.size() );
}
