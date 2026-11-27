/**
 * @file OpenGLObjectTests.cpp
 * @brief Functional tests for OpenGL backend object identity helpers.
 */

#include <RHIOpenGL/Types/OpenGLObject.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    namespace
    {

        class FTestOpenGLObjectA final
        {
        };

        class FTestOpenGLObjectB final
        {
        };

    } // namespace

    LUMEN_TEST( OpenGLObject, BackendTypeTagIsStablePerType )
    {
        constexpr UInt64 FirstTag  = GetOpenGLBackendTypeTag<FTestOpenGLObjectA>();
        constexpr UInt64 SecondTag = GetOpenGLBackendTypeTag<FTestOpenGLObjectA>();
        LUMEN_CHECK_EQ( FirstTag, SecondTag );
    }

    LUMEN_TEST( OpenGLObject, DifferentTypesHaveDifferentTags )
    {
        constexpr UInt64 TagA = GetOpenGLBackendTypeTag<FTestOpenGLObjectA>();
        constexpr UInt64 TagB = GetOpenGLBackendTypeTag<FTestOpenGLObjectB>();
        LUMEN_CHECK_NOT_EQ( TagA, TagB );
    }

    LUMEN_TEST( OpenGLObject, MakeOpenGLObjectBaseStoresIdAndTag )
    {
        constexpr UInt64 ObjectValue     = 123ULL;
        const RHI::FRHIObjectBase Object = MakeOpenGLObjectBase<FTestOpenGLObjectA>( RHI::FRHIObjectId{ ObjectValue } );
        LUMEN_CHECK_EQ( Object.GetId().GetValue(), ObjectValue );
        LUMEN_CHECK_EQ( Object.GetBackendTypeTag(), GetOpenGLBackendTypeTag<FTestOpenGLObjectA>() );
    }

} // namespace RHIOpenGL

} // namespace Lumen
