/**
 * @file DLSLParser.cpp
 * @brief Implementation of the DLSL Parser.
 */

#include "Container/Expected.hpp"

#include "LumenCompiler/Internal/DLSLParser.hpp"

#include <charconv>
#include <format>

/**
 * Ctor
 */

LumenEngine::Compiler::FDLSLParser::FDLSLParser ( FStringView InSource, HAL::FLinearAllocator &InAllocator ) noexcept : Scanner( InSource ), Allocator( InAllocator )
{
    Advance();
}

/**
 * Public Methods
 */

namespace
{

void AppendPropertyToObject ( LumenEngine::Compiler::FDLSLNode *InOutObjectNode, LumenEngine::Compiler::FDLSLProperty *InProperty ) noexcept
{
    if ( InOutObjectNode->ObjectValue.Head == nullptr )
    {
        InOutObjectNode->ObjectValue.Head = InProperty;
    }
    else
    {
        InOutObjectNode->ObjectValue.Tail->Next = InProperty;
    }

    InOutObjectNode->ObjectValue.Tail = InProperty;
    ++InOutObjectNode->ObjectValue.Count;
}

void AppendNodeToList ( LumenEngine::Compiler::FDLSLNode *InOutListNode, LumenEngine::Compiler::FDLSLNode *InValueNode ) noexcept
{
    if ( InOutListNode->ListValue.Head == nullptr )
    {
        InOutListNode->ListValue.Head = InValueNode;
    }
    else
    {
        InOutListNode->ListValue.Tail->Next = InValueNode;
    }

    InOutListNode->ListValue.Tail = InValueNode;
    ++InOutListNode->ListValue.Count;
}

} // namespace

LumenEngine::TExpected<LumenEngine::Compiler::FDLSLDocument *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::Parse ()
{
    const TExpected<FDLSLDocument *, FString> DocumentResult = AllocateNode<FDLSLDocument>();
    LUMEN_EXPECT_VALUE( DocumentResult );

    FDLSLDocument *const Document = DocumentResult.value();
    FDLSLRootBlock *CurrentBlock  = nullptr;

    while ( Peek().Type != FToken::EType::EndOfFile )
    {
        const TExpected<FDLSLRootBlock *, FString> BlockResult = ParseRootBlock();
        LUMEN_EXPECT_VALUE( BlockResult );

        if ( CurrentBlock == nullptr )
        {
            Document->FirstBlock = BlockResult.value();
            CurrentBlock         = Document->FirstBlock;
        }
        else
        {
            CurrentBlock->Next = BlockResult.value();
            CurrentBlock       = CurrentBlock->Next;
        }
    }

    return Document;
}

/**
 * Private Parsers
 */

LumenEngine::TExpected<LumenEngine::Compiler::FDLSLRootBlock *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseRootBlock ()
{
    const TExpected<FDLSLRootBlock *, FString> BlockResult = AllocateNode<FDLSLRootBlock>();
    LUMEN_EXPECT_VALUE( BlockResult );

    FDLSLRootBlock *const Block = BlockResult.value();

    const TExpected<FToken, FString> AtTokenResult = Consume( FToken::EType::At, "Expected '@' before root block type." );
    LUMEN_EXPECT_VALUE( AtTokenResult );

    const TExpected<FToken, FString> TypeTokenResult = Consume( FToken::EType::Identifier, "Expected identifier after '@'." );
    LUMEN_EXPECT_VALUE( TypeTokenResult );

    Block->BlockType = TypeTokenResult.value().Text;

    if ( Peek().Type == FToken::EType::String )
    {
        Block->Name = Advance().Text;
    }

    const TExpected<FDLSLNode *, FString> BodyResult = ParseObject();
    LUMEN_EXPECT_VALUE( BodyResult );

    Block->Body = BodyResult.value();

    return Block;
}

// NOLINTBEGIN(misc-no-recursion)
LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseNode ()
{
    switch ( Peek().Type )
    {
    case FToken::EType::LBrace:
        return ParseObject();
    case FToken::EType::LBracket:
        return ParseList();
    case FToken::EType::LParen:
        return ParseVector();
    case FToken::EType::String:
        return ParseStringNode();
    case FToken::EType::Number:
        return ParseNumberNode();
    case FToken::EType::Identifier:
        return ParseIdentifierOrBooleanNode();
    default:
        break;
    }

    return MakeUnexpected( std::format( "Parse Error [Line {}]: Unexpected token type.", Peek().Line ) );
}
// NOLINTEND(misc-no-recursion)

LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseStringNode ()
{
    const TExpected<FDLSLNode *, FString> NodeResult = AllocateNode<FDLSLNode>();
    LUMEN_EXPECT_VALUE( NodeResult );

    Advance();

    FDLSLNode *const Node = NodeResult.value();
    Node->Type            = EDLSLNodeType::String;
    Node->StringRaw.Data  = PreviousToken.Text.data();
    Node->StringRaw.Size  = PreviousToken.Text.size();

    return Node;
}

LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseNumberNode ()
{
    const TExpected<FDLSLNode *, FString> NodeResult = AllocateNode<FDLSLNode>();
    LUMEN_EXPECT_VALUE( NodeResult );

    Advance();

    FDLSLNode *const Node = NodeResult.value();
    Node->Type            = EDLSLNodeType::Number;

    Float64 Value = 0.0;
    std::from_chars( PreviousToken.Text.data(), PreviousToken.Text.data() + PreviousToken.Text.size(), Value );
    Node->NumberValue = Value;

    return Node;
}

LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseIdentifierOrBooleanNode ()
{
    const TExpected<FDLSLNode *, FString> NodeResult = AllocateNode<FDLSLNode>();
    LUMEN_EXPECT_VALUE( NodeResult );

    Advance();

    FDLSLNode *const Node = NodeResult.value();

    if ( PreviousToken.Text == "True" )
    {
        Node->Type         = EDLSLNodeType::Boolean;
        Node->BooleanValue = true;
    }
    else if ( PreviousToken.Text == "False" )
    {
        Node->Type         = EDLSLNodeType::Boolean;
        Node->BooleanValue = false;
    }
    else
    {
        Node->Type               = EDLSLNodeType::Identifier;
        Node->IdentifierRaw.Data = PreviousToken.Text.data();
        Node->IdentifierRaw.Size = PreviousToken.Text.size();
    }

    return Node;
}

// NOLINTBEGIN(misc-no-recursion)
LumenEngine::TExpected<LumenEngine::Compiler::FDLSLProperty *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseProperty ()
{
    FStringView PropertyKey;

    if ( Match( FToken::EType::At ) )
    {
        const TExpected<FToken, FString> IdentifierResult = Consume( FToken::EType::Identifier, "Expected identifier after '@'." );
        LUMEN_EXPECT_VALUE( IdentifierResult );

        PropertyKey = FStringView( IdentifierResult.value().Text.data() - 1, IdentifierResult.value().Text.size() + 1 );
    }
    else
    {
        const TExpected<FToken, FString> IdentifierResult = Consume( FToken::EType::Identifier, "Expected property key." );
        LUMEN_EXPECT_VALUE( IdentifierResult );

        PropertyKey = IdentifierResult.value().Text;

        if ( not Match( FToken::EType::Colon ) and not Match( FToken::EType::Equals ) )
        {
            /* Legal: Nested configuration blocks might lack assignment operators */
        }
    }

    const TExpected<FDLSLNode *, FString> ValueResult = ParseNode();
    LUMEN_EXPECT_VALUE( ValueResult );

    const TExpected<FDLSLProperty *, FString> PropertyResult = AllocateNode<FDLSLProperty>();
    LUMEN_EXPECT_VALUE( PropertyResult );

    FDLSLProperty *const Property = PropertyResult.value();
    Property->Key                 = PropertyKey;
    Property->Value               = ValueResult.value();

    return Property;
}
// NOLINTEND(misc-no-recursion)

// NOLINTBEGIN(misc-no-recursion)
LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseObject ()
{
    const TExpected<FToken, FString> LBraceResult = Consume( FToken::EType::LBrace, "Expected '{' to start object." );
    LUMEN_EXPECT_VALUE( LBraceResult );

    const TExpected<FDLSLNode *, FString> NodeResult = AllocateNode<FDLSLNode>();
    LUMEN_EXPECT_VALUE( NodeResult );

    FDLSLNode *const ObjectNode   = NodeResult.value();
    ObjectNode->Type              = EDLSLNodeType::Object;
    ObjectNode->ObjectValue.Head  = nullptr;
    ObjectNode->ObjectValue.Tail  = nullptr;
    ObjectNode->ObjectValue.Count = 0;

    while ( Peek().Type != FToken::EType::RBrace and Peek().Type != FToken::EType::EndOfFile )
    {
        const TExpected<FDLSLProperty *, FString> PropertyResult = ParseProperty();
        LUMEN_EXPECT_VALUE( PropertyResult );

        AppendPropertyToObject( ObjectNode, PropertyResult.value() );
        Match( FToken::EType::Comma );
    }

    const TExpected<FToken, FString> RBraceResult = Consume( FToken::EType::RBrace, "Expected '}' to end object." );
    LUMEN_EXPECT_VALUE( RBraceResult );

    return ObjectNode;
}
// NOLINTEND(misc-no-recursion)

// NOLINTBEGIN(misc-no-recursion)
LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseList ()
{
    const TExpected<FToken, FString> LBracketResult = Consume( FToken::EType::LBracket, "Expected '[' to start list." );
    LUMEN_EXPECT_VALUE( LBracketResult );

    const TExpected<FDLSLNode *, FString> NodeResult = AllocateNode<FDLSLNode>();
    LUMEN_EXPECT_VALUE( NodeResult );

    FDLSLNode *const ListNode = NodeResult.value();
    ListNode->Type            = EDLSLNodeType::List;
    ListNode->ListValue.Head  = nullptr;
    ListNode->ListValue.Tail  = nullptr;
    ListNode->ListValue.Count = 0;

    while ( Peek().Type != FToken::EType::RBracket and Peek().Type != FToken::EType::EndOfFile )
    {
        const TExpected<FDLSLNode *, FString> ValueResult = ParseNode();
        LUMEN_EXPECT_VALUE( ValueResult );

        AppendNodeToList( ListNode, ValueResult.value() );
        Match( FToken::EType::Comma );
    }

    const TExpected<FToken, FString> RBracketResult = Consume( FToken::EType::RBracket, "Expected ']' to end list." );
    LUMEN_EXPECT_VALUE( RBracketResult );

    return ListNode;
}
// NOLINTEND(misc-no-recursion)

LumenEngine::TExpected<LumenEngine::Compiler::FDLSLNode *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::ParseVector ()
{
    const TExpected<FToken, FString> LParenResult = Consume( FToken::EType::LParen, "Expected '(' to start vector." );
    LUMEN_EXPECT_VALUE( LParenResult );

    const TExpected<FDLSLNode *, FString> NodeResult = AllocateNode<FDLSLNode>();
    LUMEN_EXPECT_VALUE( NodeResult );

    FDLSLNode *const VectorNode   = NodeResult.value();
    VectorNode->Type              = EDLSLNodeType::Vector;
    VectorNode->VectorValue.Count = 0;

    while ( Peek().Type != FToken::EType::RParen and Peek().Type != FToken::EType::EndOfFile )
    {
        const TExpected<FToken, FString> NumberResult = Consume( FToken::EType::Number, "Expected number in vector." );
        LUMEN_EXPECT_VALUE( NumberResult );

        Float64 Value = 0.0;
        std::from_chars( NumberResult.value().Text.data(), NumberResult.value().Text.data() + NumberResult.value().Text.size(), Value );

        if ( VectorNode->VectorValue.Count < 4 )
        {
            VectorNode->VectorValue.Data[VectorNode->VectorValue.Count] = static_cast<Float32>( Value );
            ++VectorNode->VectorValue.Count;
        }

        Match( FToken::EType::Comma );
    }

    const TExpected<FToken, FString> RParenResult = Consume( FToken::EType::RParen, "Expected ')' to end vector." );
    LUMEN_EXPECT_VALUE( RParenResult );

    return VectorNode;
}

/**
 * Private Helper Methods
 */

template <typename Type, typename... Args> LumenEngine::TExpected<Type *, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::AllocateNode ( Args &&...InArgs )
{
    void *const Memory = Allocator.Allocate( sizeof( Type ), alignof( Type ) );

    if ( Memory == nullptr )
    {
        return MakeUnexpected( "Out of memory during parsing AST nodes." );
    }

    return new ( Memory ) Type( std::forward<Args>( InArgs )... );
}

LumenEngine::Compiler::FToken LumenEngine::Compiler::FDLSLParser::Advance () noexcept
{
    PreviousToken = CurrentToken;
    CurrentToken  = Scanner.ScanToken();
    return PreviousToken;
}

LumenEngine::Compiler::FToken LumenEngine::Compiler::FDLSLParser::Peek () const noexcept
{
    return CurrentToken;
}

LumenEngine::Bool LumenEngine::Compiler::FDLSLParser::Match ( FToken::EType InType ) noexcept
{
    if ( Peek().Type == InType )
    {
        Advance();
        return true;
    }

    return false;
}

LumenEngine::TExpected<LumenEngine::Compiler::FToken, LumenEngine::FString> LumenEngine::Compiler::FDLSLParser::Consume ( FToken::EType InType,
                                                                                                                          FStringView InErrorMessage ) noexcept
{
    if ( Peek().Type == InType )
    {
        return Advance();
    }

    try
    {
        return MakeUnexpected( std::format( "Parse Error [Line {}]: {}", Peek().Line, InErrorMessage ) );
    }
    catch ( const std::format_error &FormatError )
    {
        return MakeUnexpected( FormatError.what() );
    }
}
