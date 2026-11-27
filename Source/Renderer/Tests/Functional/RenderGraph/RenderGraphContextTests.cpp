/**
 * @file RenderGraphContextTests.cpp
 * @brief Ownership and validation contracts of the render graph execution context.
 */

#include <RenderGraph/RenderGraphBuilder.hpp>
#include <RenderGraph/RenderGraphContext.hpp>

#include <Tests/UnitTest.hpp>

#include <type_traits>
#include <utility>

namespace Lumen
{

LUMEN_TEST( FRenderGraphContext, OwnsCompiledGraphMetadata )
{
    static_assert( not std::is_copy_constructible_v<FRenderGraphContext> );
    static_assert( not std::is_copy_assignable_v<FRenderGraphContext> );

    /** INFO: The source expected and builder die inside the lambda; the context must keep the compiled metadata alive. */
    FRenderGraphContext Context = [&] () noexcept
    {
        FRenderGraphBuilder Builder( FRenderGraphCapacities{} );
        auto Compiled = std::move( Builder ).Compile();
        LUMEN_ASSERT( Compiled.HasValue() );
        return FRenderGraphContext( std::move( *Compiled ) );
    }();

    const FRenderResourceHandle Missing{ .Index = 0U, .Generation = 1U };
    const auto Binding = Context.GetResourceBinding( Missing );
    LUMEN_CHECK_HAS_ERROR( Binding );
    LUMEN_CHECK_EQ( Binding.Error().Code, ERenderErrorCode::InvalidResourceHandle );
}

} // namespace Lumen
