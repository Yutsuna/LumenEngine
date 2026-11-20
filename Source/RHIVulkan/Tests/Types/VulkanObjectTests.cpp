/**
 * @file VulkanObjectTests.cpp
 * @brief Functional tests for Vulkan backend object identity helpers.
 */

#include <Tests/UnitTest.hpp>
#include <Types/VulkanObject.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    namespace
    {

        class FTestVulkanObjectA final
        {
        };

        class FTestVulkanObjectB final
        {
        };

    } // namespace

    LUMEN_TEST( VulkanObject, BackendTypeTagIsStablePerType )
    {
        constexpr UInt64 FirstTag  = GetVulkanBackendTypeTag<FTestVulkanObjectA>();
        constexpr UInt64 SecondTag = GetVulkanBackendTypeTag<FTestVulkanObjectA>();
        LUMEN_CHECK_EQ( FirstTag, SecondTag );
    }

    LUMEN_TEST( VulkanObject, DifferentTypesHaveDifferentTags )
    {
        constexpr UInt64 TagA = GetVulkanBackendTypeTag<FTestVulkanObjectA>();
        constexpr UInt64 TagB = GetVulkanBackendTypeTag<FTestVulkanObjectB>();
        LUMEN_CHECK_NOT_EQ( TagA, TagB );
    }

    LUMEN_TEST( VulkanObject, MakeVulkanObjectBaseStoresIdAndTag )
    {
        constexpr UInt64 ObjectValue     = 123ULL;
        const RHI::FRHIObjectBase Object = MakeVulkanObjectBase<FTestVulkanObjectA>( RHI::FRHIObjectId{ ObjectValue } );
        LUMEN_CHECK_EQ( Object.GetId().GetValue(), ObjectValue );
        LUMEN_CHECK_EQ( Object.GetBackendTypeTag(), GetVulkanBackendTypeTag<FTestVulkanObjectA>() );
    }

} // namespace RHIVulkan

} // namespace Lumen
