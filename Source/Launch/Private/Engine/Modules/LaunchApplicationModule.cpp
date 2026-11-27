/**
 * @file LaunchApplicationModule.cpp
 * @brief FLaunchApplicationModule implementation.
 */

#include <Core/CoreDefinitions.hpp>

#include <Engine/LaunchEngineLoop.hpp>
#include <Engine/Modules/LaunchApplicationModule.hpp>

#include <Generic/GenericApplication.hpp>

#if defined( LUMEN_PLATFORM_LINUX )
    #include <Linux/LinuxApplication.hpp>
#endif

#include <cstddef>
#include <utility>

/**
 * Registration
 */

LUMEN_REGISTER_MODULE( Lumen::Launch::FLaunchApplicationModule )

/**
 * Internal
 */

struct Lumen::Launch::Internal::FMessageHandlerNode
{
    explicit FMessageHandlerNode ( const TSharedRef<FGenericApplicationMessageHandler> &InHandler ) noexcept
        : Handler( InHandler )
    {
        /** Empty */
    }

    TSharedRef<FGenericApplicationMessageHandler> Handler;
};

/**
 * FScopedMessageHandler
 */

Lumen::Launch::FScopedMessageHandler::FScopedMessageHandler ( FLaunchApplicationModule *const InOwner,
                                                              const Internal::FMessageHandlerNode *const InToken ) noexcept
    : Owner( InOwner ), Token( InToken )
{
    /** Empty */
}

Lumen::Launch::FScopedMessageHandler::FScopedMessageHandler ( FScopedMessageHandler &&Other ) noexcept
    : Owner( Other.Owner ), Token( Other.Token )
{
    Other.Owner = nullptr;
    Other.Token = nullptr;
}

Lumen::Launch::FScopedMessageHandler &Lumen::Launch::FScopedMessageHandler::operator=( FScopedMessageHandler &&Other ) noexcept
{
    if ( this != &Other )
    {
        Reset();

        Owner       = Other.Owner;
        Token       = Other.Token;
        Other.Owner = nullptr;
        Other.Token = nullptr;
    }
    return *this;
}

Lumen::Launch::FScopedMessageHandler::~FScopedMessageHandler () noexcept
{
    Reset();
}

void Lumen::Launch::FScopedMessageHandler::Reset () noexcept
{
    if ( Owner != nullptr and Token != nullptr )
    {
        Owner->PopMessageHandler( Token );
    }

    Owner = nullptr;
    Token = nullptr;
}

/**
 * FLaunchMessageHandler
 */

void Lumen::Launch::FLaunchMessageHandler::OnRequestExit () noexcept
{
    FLaunchEngineLoop::RequestExit();
}

void Lumen::Launch::FLaunchMessageHandler::OnQuit () noexcept
{
    FLaunchEngineLoop::RequestExit();
}

/**
 * FLaunchApplicationModule
 */

Lumen::Bool Lumen::Launch::FLaunchApplicationModule::IsEnabled ( const FLaunchEngineConfig &InConfig ) noexcept
{
    return not InConfig.bHeadless;
}

Lumen::Launch::TLaunchResult<Lumen::TUniquePtr<Lumen::Launch::FLaunchApplicationModule>>
Lumen::Launch::FLaunchApplicationModule::Create (
    const TModuleContext<FLaunchApplicationModule> LUMEN_UNUSED &InContext ) noexcept
{
#if defined( LUMEN_PLATFORM_LINUX )
    const TSharedPtr<FGenericApplication> PlatformApplication = FLinuxApplication::MakeLinuxApplication();
    if ( not PlatformApplication.IsValid() ) [[unlikely]]
    {
        return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                              "Failed to create platform application" );
    }

    return MakeUnique<FLaunchApplicationModule>( PlatformApplication );
#else
    return LaunchFailure( ELaunchStage::Init, ELaunchErrorCode::ModuleCreationFailed,
                          "No platform application implementation for this target" );
#endif
}

Lumen::Launch::FLaunchApplicationModule::FLaunchApplicationModule ( TSharedPtr<FGenericApplication> InApplication ) noexcept
    : Application( std::move( InApplication ) ), MessageHandler( MakeShared<FLaunchMessageHandler>() )
{
    HandlerStack.push_back( MakeUnique<Internal::FMessageHandlerNode>( MessageHandler ) );

    if ( Application.IsValid() )
    {
        Application->SetMessageHandler( MessageHandler );
    }
}

Lumen::Launch::FLaunchApplicationModule::~FLaunchApplicationModule () noexcept
{
    if ( Application.IsValid() )
    {
        Application->Shutdown();
        Application.Reset();
    }

    HandlerStack.clear();
}

void Lumen::Launch::FLaunchApplicationModule::Tick ( const Float64 InDeltaSeconds, const Float64 LUMEN_UNUSED InAlpha ) noexcept
{
    if ( Application.IsValid() )
    {
        Application->PumpMessages( InDeltaSeconds );
    }
}

Lumen::Launch::EModuleTraits Lumen::Launch::FLaunchApplicationModule::GetTraits () const noexcept
{
    return EModuleTraits::Tick;
}

const Lumen::TSharedPtr<Lumen::FGenericApplication> &Lumen::Launch::FLaunchApplicationModule::GetApplication () const noexcept
{
    return Application;
}

Lumen::TSharedPtr<Lumen::FGenericApplicationMessageHandler>
Lumen::Launch::FLaunchApplicationModule::GetCurrentMessageHandler () const noexcept
{
    if ( not HandlerStack.empty() )
    {
        return { HandlerStack.back()->Handler };
    }

    if ( Application.IsValid() )
    {
        return Application->GetMessageHandler();
    }

    return {};
}

Lumen::Launch::FScopedMessageHandler Lumen::Launch::FLaunchApplicationModule::InstallMessageHandler (
    const TSharedRef<FGenericApplicationMessageHandler> &InHandler ) noexcept
{
    if ( not Application.IsValid() ) [[unlikely]]
    {
        return {};
    }

    TUniquePtr<Internal::FMessageHandlerNode> Node   = MakeUnique<Internal::FMessageHandlerNode>( InHandler );
    const Internal::FMessageHandlerNode *const Token = Node.Get();

    HandlerStack.push_back( std::move( Node ) );
    Application->SetMessageHandler( HandlerStack.back()->Handler );

    return { this, Token };
}

void Lumen::Launch::FLaunchApplicationModule::PopMessageHandler ( const Internal::FMessageHandlerNode *const InToken ) noexcept
{
    if ( InToken == nullptr ) [[unlikely]]
    {
        return;
    }

    for ( USize Index = HandlerStack.size(); Index > 1; --Index )
    {
        const USize EntryIndex = Index - 1;

        if ( HandlerStack[EntryIndex].Get() == InToken )
        {
            const Bool bWasTop = ( EntryIndex + 1 == HandlerStack.size() );
            HandlerStack.erase( HandlerStack.begin() + static_cast<std::ptrdiff_t>( EntryIndex ) );

            if ( bWasTop and Application.IsValid() and not HandlerStack.empty() )
            {
                Application->SetMessageHandler( HandlerStack.back()->Handler );
            }
            return;
        }
    }
}
