//---------------------------------------------------------------------------------------------------------------------------
// Nodlang.cpp
// This file is structured in 3 parts, use Ctrl + F to search:
//  [SECTION] A. Declaration (types, keywords, etc.)
//  [SECTION] B. Parser
//  [SECTION] C. Serializer
//---------------------------------------------------------------------------------------------------------------------------

#include "Nodlang.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <chrono>
#include <cctype> // isdigit, isalpha, and isalnum.

#include "tools/core/reflection/reflection"
#include "tools/core/format.h"
#include "tools/core/log.h"
#include "tools/core/Hash.h"

#include "ndbl/core/ASTUtils.h"
#include "ndbl/core/ASTSlotLink.h"
#include "ndbl/core/ASTForLoop.h"
#include "ndbl/core/Graph.h"
#include "ndbl/core/ASTIf.h"
#include "ndbl/core/ASTFunctionCall.h"
#include "ndbl/core/ASTLiteral.h"
#include "ndbl/core/ASTNodeProperty.h"
#include "ndbl/core/ASTScope.h"
#include "ndbl/core/ASTVariable.h"
#include "ndbl/core/ASTVariableRef.h"
#include "ndbl/core/ASTWhileLoop.h"

using namespace ndbl;
using namespace tools;

static Nodlang* g_language{ nullptr };

//---------------------------------------------------------------------------------------------------------------------------
// [SECTION] A. Declaration -------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

Nodlang::Nodlang(bool _strict)
    : m_strict_mode(_strict)
    , _state()
{
    // A.1. Define the language
    //-------------------------
    m_definition.chars =
    {
        { '(',  ASTToken_t::parenthesis_open},
        { ')',  ASTToken_t::parenthesis_close},
        { '{',  ASTToken_t::scope_begin},
        { '}',  ASTToken_t::scope_end},
        { '\n', ASTToken_t::ignore},
        { '\t', ASTToken_t::ignore},
        { ' ',  ASTToken_t::ignore},
        { ';',  ASTToken_t::end_of_instruction},
        { ',',  ASTToken_t::list_separator}
    };

    m_definition.keywords =
    {
         { "if",       ASTToken_t::keyword_if },
         { "for",      ASTToken_t::keyword_for },
         { "while",    ASTToken_t::keyword_while },
         { "else",     ASTToken_t::keyword_else },
         { "true",     ASTToken_t::literal_bool },
         { "false",    ASTToken_t::literal_bool },
         { "operator", ASTToken_t::keyword_operator },
    };

    m_definition.types =
    {
         { "bool",   ASTToken_t::keyword_bool,   type::get<bool>()},
         { "string", ASTToken_t::keyword_string, type::get<std::string>()},
         { "double", ASTToken_t::keyword_double, type::get<double>()},
         { "i16",    ASTToken_t::keyword_i16,    type::get<i16_t>()},
         { "int",    ASTToken_t::keyword_int,    type::get<i32_t>()},
         { "any",    ASTToken_t::keyword_any,    type::get<any>()},
         // we don't really want to parse/serialize that
         // { "unknown",Token_t::keyword_unknown,type::get<unknown>()},
    };

    m_definition.operators =
    {
         {"-",   Operator_t::Unary,   5},
         {"!",   Operator_t::Unary,   5},
         {"/",   Operator_t::Binary, 20},
         {"*",   Operator_t::Binary, 20},
         {"+",   Operator_t::Binary, 10},
         {"-",   Operator_t::Binary, 10},
         {"||",  Operator_t::Binary, 10},
         {"&&",  Operator_t::Binary, 10},
         {">=",  Operator_t::Binary, 10},
         {"<=",  Operator_t::Binary, 10},
         {"=>",  Operator_t::Binary, 10},
         {"==",  Operator_t::Binary, 10},
         {"<=>", Operator_t::Binary, 10},
         {"!=",  Operator_t::Binary, 10},
         {">",   Operator_t::Binary, 10},
         {"<",   Operator_t::Binary, 10},
         {"=",   Operator_t::Binary,  0},
         {"+=",  Operator_t::Binary,  0},
         {"-=",  Operator_t::Binary,  0},
         {"/=",  Operator_t::Binary,  0},
         {"*=",  Operator_t::Binary,  0}
    };

    // A.2. Create indexes
    //---------------------
    for( auto [_char, token_t] : m_definition.chars)
    {
        m_token_t_by_single_char.insert({_char, token_t});
        m_single_char_by_keyword.insert({token_t, _char});
    }

    for( auto [keyword, token_t] : m_definition.keywords)
    {
        m_token_t_by_keyword.insert({Hash::hash(keyword), token_t});
        m_keyword_by_token_t.insert({token_t, keyword});
    }

    for( auto [keyword, token_t, type] : m_definition.types)
    {
        m_keyword_by_token_t.insert({token_t, keyword});
        m_keyword_by_type_id.insert({type->id(), keyword});
        m_token_t_by_keyword.insert({Hash::hash(keyword), token_t});
        m_token_t_by_type_id.insert({type->id(), token_t});
        m_type_by_token_t.insert({token_t, type});
    }

    for( auto [keyword, operator_t, precedence] : m_definition.operators)
    {
        const Operator *op = new Operator(keyword, operator_t, precedence);
        ASSERT(std::find(m_operators.begin(), m_operators.end(), op) == m_operators.end());
        m_operators.push_back(op);
    }
}

Nodlang::~Nodlang()
{
    for(const Operator* each : m_operators )
        delete each;

//    for(const IInvokable* each : m_functions ) // static and member functions are owned by their respective tools::type<T>
//        delete each;
}
//---------------------------------------------------------------------------------------------------------------------------
// [SECTION] B. Parser ------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

bool Nodlang::parse(Graph* graph_out, const std::string& code)
{
    _state.reset(graph_out);

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing ...\n%s\n", code.c_str());

    if ( !tokenize(code) )
    {
        return false;
    }

    if (!is_syntax_valid())
    {
        return false;
    }

    ASTScope* scope = parse_program();

    if ( scope->empty(ScopeFlags_RECURSE_CHILD_PARTITION) )
    {
        return false;
    }

    if (_state.tokens().can_eat() )
    {
        _state.graph()->reset();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " End of token ribbon expected\n");
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "%s", format::title("TokenRibbon").c_str());
        for (const ASTToken& each_token : _state.tokens() )
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "token idx %i: %s\n", each_token.m_index, each_token.json().c_str());
        }
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "%s", format::title("TokenRibbon end").c_str());
        auto curr_token = _state.tokens().peek();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Failed to parse from token %llu/%llu and above.\n", curr_token.m_index, _state.tokens().size());
        TOOLS_LOG(tools::Verbosity_Error, "Parser", "Unable to parse all the tokens\n");
        return false;
    }
    return true;
}

bool Nodlang::parse_bool_or(const std::string &_str, bool default_value) const
{
    size_t cursor = 0;
    ASTToken  token  = parse_token(_str.c_str(), _str.size(), cursor);
    if (token.m_type == ASTToken_t::literal_bool )
        return _str == std::string("true");
    return default_value;
}

std::string Nodlang::remove_quotes(const std::string &_quoted_str) const
{
    ASSERT(_quoted_str.size() >= 2);
    ASSERT(_quoted_str.front() == '\"');
    ASSERT(_quoted_str.back() == '\"');
    return std::string(++_quoted_str.cbegin(), --_quoted_str.cend());
}

double Nodlang::parse_double_or(const std::string &_str, double default_value) const
{
    size_t cursor = 0;
    ASTToken  token  = parse_token(_str.c_str(), _str.size(), cursor);
    if (token.m_type == ASTToken_t::literal_double )
        return std::stod(_str);
    return default_value;
}


int Nodlang::parse_int_or(const std::string &_str, int default_value) const
{
    size_t cursor = 0;
    ASTToken  token  = parse_token(_str.c_str(), _str.size(), cursor);
    if (token.m_type == ASTToken_t::literal_int )
        return stoi(_str);
    return default_value;
}

ASTNodeSlot* Nodlang::token_to_slot(ASTScope* parent_scope, const ASTToken& _token)
{
    if (_token.m_type == ASTToken_t::identifier)
    {
        std::string identifier = _token.word_to_string();
        if( ASTVariable* existing_variable = parent_scope->find_variable(identifier) )
        {
            return existing_variable->ref_out();
        }

        if ( !m_strict_mode )
        {
            // Insert a VariableNodeRef with "any" type
            TOOLS_LOG(tools::Verbosity_Warning,  "Parser", "%s is not declared (strict mode), abstract graph can be generated but compilation will fail.\n",
                         _token.word_to_string().c_str() );
            ASTVariableRef* ref = _state.graph()->create_variable_ref( parent_scope );
            ref->value()->set_token(_token );
            return ref->value_out();
        }

        TOOLS_LOG(tools::Verbosity_Error,  "Parser", "%s is not declared (strict mode) \n", _token.word_to_string().c_str() );
        return nullptr;
    }

    ASTLiteral* literal{};

    switch (_token.m_type)
    {
        case ASTToken_t::literal_bool:   literal = _state.graph()->create_literal<bool>( parent_scope );        break;
        case ASTToken_t::literal_int:    literal = _state.graph()->create_literal<i32_t>( parent_scope );       break;
        case ASTToken_t::literal_double: literal = _state.graph()->create_literal<double>( parent_scope );      break;
        case ASTToken_t::literal_string: literal = _state.graph()->create_literal<std::string>( parent_scope ); break;
        default:
            break; // we don't want to throw
    }

    if ( literal )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Token %s converted to a Literal %s\n",
                    _token.word_to_string().c_str(),
                    literal->value()->get_type()->name());
        literal->value()->set_token( _token );
        return literal->value_out();
    }

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Unable to run token_to_slot with token %s!\n", _token.word_to_string().c_str());
    return nullptr;
}

ASTNodeSlot* Nodlang::parse_binary_operator_expression(ASTScope* parent_scope, u8_t _precedence, ASTNodeSlot* _left)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing binary expression ...\n");
    ASSERT(_left != nullptr);

    if (!_state.tokens().can_eat(2))
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Not enough tokens\n");
        return nullptr;
    }

    _state.start_transaction();
    const ASTToken operator_token = _state.tokens().eat();
    const ASTToken operand_token  = _state.tokens().peek();

    // Structure check
    const bool isValid = operator_token.m_type == ASTToken_t::operator_ &&
                         operand_token.m_type != ASTToken_t::operator_;

    if (!isValid)
    {
        _state.rollback();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Unexpected tokens\n");
        return nullptr;
    }

    std::string word = operator_token.word_to_string();  // FIXME: avoid std::string copy, use hash
    const Operator *ope = find_operator(word, Operator_t::Binary);
    if (ope == nullptr)
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Operator %s not found\n", word.c_str());
        _state.rollback();
        return nullptr;
    }

    // Precedence check
    if (ope->precedence <= _precedence && _precedence > 0)
    {// always update the first operation if they have the same precedence or less.
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Has lower precedence\n");
        _state.rollback();
        return nullptr;
    }

    // Parse right expression
    if ( ASTNodeSlot* right = parse_expression(parent_scope, ope->precedence) )
    {
        // Create a function signature according to ltype, rtype and operator word
        FunctionDescriptor type;
        type.init<any(any, any)>(ope->identifier.c_str());
        type.arg_at(0).type = _left->property->get_type();
        type.arg_at(1).type = right->property->get_type();

        ASTFunctionCall* binary_op = _state.graph()->create_operator( type, _left->node->scope() );
        binary_op->set_identifier_token( operator_token );
        binary_op->lvalue_in()->property->token().m_type = _left->property->token().m_type;
        binary_op->rvalue_in()->property->token().m_type = right->property->token().m_type;

        _state.graph()->connect_or_merge(_left, binary_op->lvalue_in());
        _state.graph()->connect_or_merge(right, binary_op->rvalue_in() );

        _state.commit();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Binary expression parsed:\n%s\n", _state.tokens().to_string().c_str());
        return binary_op->value_out();
    }

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Right expression is null\n");
    _state.rollback();
    return nullptr;
}

ASTNodeSlot* Nodlang::parse_unary_operator_expression(ASTScope* parent_scope, u8_t _precedence)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "parseUnaryOperationExpression...\n");

    if (!_state.tokens().can_eat(2))
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Not enough tokens\n");
        return nullptr;
    }

    _state.start_transaction();
    ASTToken operator_token = _state.tokens().eat();

    // Check if we get an operator first
    if (operator_token.m_type != ASTToken_t::operator_)
    {
        _state.rollback();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Expecting an operator token first\n");
        return nullptr;
    }

    // Parse expression after the operator
    ASTNodeSlot* out_atomic = parse_atomic_expression(parent_scope);

    if ( !out_atomic )
    {
        out_atomic = parse_parenthesis_expression( parent_scope );
    }

    if ( !out_atomic )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Right expression is null\n");
        _state.rollback();
        return nullptr;
    }

    // Create a function signature
    FunctionDescriptor type;
    type.init<any(any)>(operator_token.word_to_string().c_str());
    type.arg_at(0).type = out_atomic->property->get_type();

    ASTFunctionCall* node = _state.graph()->create_operator( type, parent_scope );
    node->set_identifier_token( operator_token );
    node->lvalue_in()->property->token().m_type = out_atomic->property->token().m_type;

    _state.graph()->connect_or_merge(out_atomic, node->lvalue_in() );

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Unary expression parsed:\n%s\n", _state.tokens().to_string().c_str());
    _state.commit();

    return node->value_out();
}

ASTNodeSlot* Nodlang::parse_atomic_expression(ASTScope* parent_scope)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing atomic expression ... \n");

    if (!_state.tokens().can_eat())
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Not enough tokens\n");
        return nullptr;
    }

    _state.start_transaction();
    ASTToken token = _state.tokens().eat();

    if (token.m_type == ASTToken_t::operator_)
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Cannot start with an operator token\n");
        _state.rollback();
        return nullptr;
    }

    if ( ASTNodeSlot* result = token_to_slot(parent_scope, token) )
    {
        _state.commit();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Atomic expression parsed:\n%s\n", _state.tokens().to_string().c_str());
        return result;
    }

    _state.rollback();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic,  "Parser", TOOLS_KO " Unable to parse token (%llu)\n", token.m_index );

    return nullptr;
}

ASTNodeSlot* Nodlang::parse_parenthesis_expression(ASTScope* parent_scope)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "parse parenthesis expr...\n");

    if (!_state.tokens().can_eat())
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " No enough tokens.\n");
        return nullptr;
    }

    _state.start_transaction();
    ASTToken currentToken = _state.tokens().eat();
    if (currentToken.m_type != ASTToken_t::parenthesis_open)
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Open bracket not found.\n");
        _state.rollback();
        return nullptr;
    }

    ASTNodeSlot* result = parse_expression(parent_scope);
    if ( result )
    {
        ASTToken token = _state.tokens().eat();
        if (token.m_type != ASTToken_t::parenthesis_close)
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "%s \n", _state.tokens().to_string().c_str());
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Parenthesis close expected\n",
                        token.word_to_string().c_str());
            _state.rollback();
        }
        else
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Parenthesis expression parsed:\n%s\n", _state.tokens().to_string().c_str());
            _state.commit();
        }
    }
    else
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " No expression after open parenthesis.\n");
        _state.rollback();
    }
    return result;
}

ASTNode* Nodlang::parse_expression_block(ASTScope* parent_scope, ASTNodeSlot* flow_out, ASTNodeSlot* value_in)
{
    _state.start_transaction();

    // Parse an expression
    ASTNodeSlot* value_out = parse_expression(parent_scope);

    // When expression value_out is a variable that is already part of the code flow,
    // we must create a variable reference
    if ( value_out && value_out->node->type() == ASTNodeType_VARIABLE )
    {
        auto variable = static_cast<ASTVariable*>( value_out->node );
        if ( ASTUtils::is_connected_to_codeflow(variable) ) // in such case, we have to reference the variable, since a given variable can't be twice (be declared twice) in the codeflow
        {
            // create a new variable reference
            ASTVariableRef* ref = _state.graph()->create_variable_ref( parent_scope );
            ref->set_variable( variable );
            // substitute value_out by variable reference's value_out
            value_out = ref->value_out();
        }
    }

    if ( !_state.tokens().can_eat() )
    {
        // we're passing here if there is no more token, which means we reached the end of file.
        // we allow an expression to end like that.
    }
    else
    {
        // However, in case there are still unparsed tokens, we expect certain type of token, otherwise we reset the result
        switch( _state.tokens().peek().m_type )
        {
            case ASTToken_t::end_of_instruction:
            case ASTToken_t::parenthesis_close:
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "End of instruction or parenthesis close: found in next token\n");
                break;
            default:
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " End of instruction or parenthesis close expected.\n");
                value_out = nullptr;
        }
    }

    // When expression value_out is null, but an input was provided,
    // we must create an empty instruction if an end_of_instruction token is found
    if (!value_out && value_in )
    {
        if (_state.tokens().peek(ASTToken_t::end_of_instruction))
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Empty expression found\n");

            ASTNode* empty_instr = _state.graph()->create_empty_instruction( parent_scope );
            value_out = empty_instr->value_out();
        }
    }

    // Ensure value_out is defined or rollback transaction
    if ( !value_out )
    {
        _state.rollback();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " parse instruction\n");
        return nullptr;
    }

    // Connects value_out to the provided input
    if ( value_in )
    {
        _state.graph()->connect( value_out, value_in, GraphFlag_ALLOW_SIDE_EFFECTS);
    }

    // Add an end_of_instruction token as suffix when needed
    if (ASTToken tok = _state.tokens().eat_if(ASTToken_t::end_of_instruction))
    {
        value_out->node->set_suffix( tok );
    }

    // Connects expression flow_in with the provided flow_out
    if ( flow_out != nullptr )
    {
        _state.graph()->connect( flow_out, value_out->node->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );
    }

    // Validate transaction
    _state.commit();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " parse instruction:\n%s\n", _state.tokens().to_string().c_str());

    return value_out->node;
}

ASTScope* Nodlang::parse_program()
{
    VERIFY(_state.graph() != nullptr, "A Graph is expected");

    _state.start_transaction();

    ASTScope* scope = _state.graph()->root_scope();

    // Parse main code block
    ASTNode* block_last_node = parse_code_block( scope, scope->node()->flow_enter() );

    // To preserve any ignored characters stored in the global token
    // we put the prefix and suffix in resp. token_begin and end.
    ASTToken& tok = _state.tokens().global_token();
    std::string prefix = tok.prefix_to_string();
    std::string suffix = tok.suffix_to_string();
    scope->token_begin.prefix_push_front(prefix.c_str() );
    scope->token_end.suffix_push_back(suffix.c_str() );

    if ( _state.tokens().can_eat( ) )
    {
        _state.rollback();
        _state.graph()->reset();
        _state.graph()->signal_is_complete.emit();
        TOOLS_LOG(tools::Verbosity_Warning, "Parser", "Some token remains after getting an empty code block\n");
        TOOLS_LOG(tools::Verbosity_Message, "Parser", "Parse program [OK]\n");
        return scope;
    }
    else if ( block_last_node == nullptr )
    {
        TOOLS_LOG(tools::Verbosity_Warning, "Parser", "Program main block is empty\n");
    }

    _state.commit();
    _state.graph()->signal_is_complete.emit();

    TOOLS_LOG(tools::Verbosity_Message, "Parser", "Parse program [OK]\n");

    return scope;
}

ASTNode* Nodlang::parse_scoped_block(ASTScope* parent_scope, ASTNodeSlot* flow_out)
{
    ASSERT(parent_scope);
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing scoped block ...\n");

    ASTToken token_begin = _state.tokens().eat_if(ASTToken_t::scope_begin);
    if ( !token_begin )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Expecting root_scope begin token\n");
        return nullptr;
    }

    _state.start_transaction();

    ASTNode* node = _state.graph()->create_scope(parent_scope);

    if ( flow_out != nullptr )
        _state.graph()->connect( flow_out, node->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );


    parse_code_block(node->internal_scope(), node->flow_enter()); // no return check, allows empty scope
    ASTToken token_end = _state.tokens().eat_if(ASTToken_t::scope_end);

    if ( token_end )
    {
        node->internal_scope()->token_begin = token_begin;
        node->internal_scope()->token_end = token_end;

        _state.commit();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Scoped block parsed:\n%s\n", _state.tokens().to_string().c_str());
        return node;
    }
    else
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Expecting close root_scope token\n");
    }

    _state.graph()->find_and_destroy(node);
    _state.rollback();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Scoped block parsed\n");
    return nullptr;
}

ASTNode* Nodlang::parse_code_block(ASTScope* parent_scope, ASTNodeSlot* flow_out)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing code block...\n" );

    //
    // Parse n atomic code blocks
    //
    _state.start_transaction();

    ASTNodeSlot* last_node_flow_out  = flow_out;
    bool     block_end_reached = false;
    size_t   block_size        = 0;

    while (_state.tokens().can_eat() && !block_end_reached )
    {
        if ( ASTNode* current_block = parse_atomic_code_block(parent_scope, last_node_flow_out) )
        {
            last_node_flow_out = current_block->flow_out();
            ++block_size;
        }
        else
        {
            block_end_reached = true;
        }
    }

    if (last_node_flow_out != nullptr && last_node_flow_out != flow_out )
    {
        _state.commit();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " parse code block:\n%s\n", _state.tokens().to_string().c_str());
        return last_node_flow_out->node;
    }

    _state.rollback();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " parse code block. Block size is %llu\n", block_size );
    return nullptr;
}

ASTNodeSlot* Nodlang::parse_expression(ASTScope* parent_scope, u8_t _precedence, ASTNodeSlot* _left_override)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing expression ...\n");

    /*
		Get the left-handed operand
	*/
    ASTNodeSlot* left = _left_override;

    if (!_state.tokens().can_eat())
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Last token reached\n");
        return left;
    }

    if ( !left ) left = parse_parenthesis_expression(parent_scope);
    if ( !left ) left = parse_unary_operator_expression(parent_scope, _precedence);
    if ( !left ) left = parse_function_call(parent_scope);
    if ( !left ) left = parse_variable_declaration(parent_scope); // nullptr => variable won't be attached on the codeflow, it's a part of an expression..
    if ( !left ) left = parse_atomic_expression(parent_scope);

    if (!_state.tokens().can_eat())
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Last token reached\n");
        return left;
    }

    if ( !left )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Left side is null, we return it\n");
        return left;
    }

    /*
		Get the right-handed operand
	*/
    ASTNodeSlot* expression_out = parse_binary_operator_expression(parent_scope, _precedence, left );
    if ( expression_out )
    {
        if (!_state.tokens().can_eat())
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Right side parsed, and last token reached\n");
            return expression_out;
        }
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Right side parsed, continue with a recursive call...\n");
        return parse_expression(parent_scope, _precedence, expression_out);
    }

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Returning left side only\n");

    return left;
}

bool Nodlang::is_syntax_valid()
{
    // TODO: optimization: is this function really useful ? It check only few things.
    //                     The parsing steps that follow (parseProgram) is doing a better check, by looking to what exist in the Language.
    bool success = true;
    auto token = _state.tokens().begin();
    short int opened = 0;

    while (token != _state.tokens().end() && success)
    {
        switch (token->m_type)
        {
            case ASTToken_t::parenthesis_open:
            {
                opened++;
                break;
            }
            case ASTToken_t::parenthesis_close:
            {
                if (opened <= 0)
                {
                    TOOLS_LOG(tools::Verbosity_Error, "Parser",
                              "Syntax Error: Unexpected close bracket after \"... %s\" (position %llu)\n",
                              _state.tokens().range_to_string(token->m_index, -10).c_str(),
                              token->offset()
                          );
                    success = false;
                }
                opened--;
                break;
            }
            default:
                break;
        }

        std::advance(token, 1);
    }

    if (opened > 0)// same opened/closed parenthesis count required.
    {
        TOOLS_LOG(tools::Verbosity_Error, "Parser", "Syntax Error: Bracket count mismatch, %i still opened.\n", opened);
        success = false;
    }

    return success;
}

bool Nodlang::tokenize(const std::string& _string)
{
    _state.reset_ribbon(const_cast<char *>(_string.data()), _string.length());
    return tokenize();
}

bool Nodlang::tokenize()
{
    TOOLS_LOG(tools::Verbosity_Diagnostic, "Parser", "Tokenization ...\n");

    size_t global_cursor       = 0;
    size_t ignored_chars_count = 0;

    while (global_cursor != _state.buffer_size() )
    {
        size_t current_cursor = global_cursor;
        ASTToken  new_token = parse_token(_state.buffer(), _state.buffer_size(), global_cursor );

        if ( !new_token )
        {
            TOOLS_LOG(tools::Verbosity_Warning, "Parser", TOOLS_KO " Unable to tokenize from \"%20s...\" (at index %llu)\n", _state.buffer_at(current_cursor), global_cursor);
            return false;
        }

        // accumulate ignored chars (see else case to know why)
        if(new_token.m_type == ASTToken_t::ignore)
        {
            if (  _state.tokens().empty() )
            {
                _state.tokens().global_token().prefix_end_grow(new_token.length() );
                continue;
            }

            ignored_chars_count += new_token.length();
            continue;
        }

        if ( ignored_chars_count )
        {
            // case 1: if token type allows it => increase last token's prefix to wrap the ignored chars
            ASTToken& back = _state.tokens().back();
            if ( accepts_suffix(back.m_type) )
            {
                back.suffix_end_grow(ignored_chars_count);
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "      \"%s\" (update) \n", back.string().c_str() );
            }
            // case 2: increase prefix of the new_token up to wrap the ignored chars
            else if ( new_token )
            {
                new_token.prefix_begin_grow(ignored_chars_count);
            }
            ignored_chars_count = 0;
        }

        _state.tokens().push(new_token);
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "%4llu) \"%s\" \n", new_token.m_index, new_token.string().c_str() );
    }

    // Append remaining ignored chars to the ribbon's suffix
    if ( ignored_chars_count )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Found ignored chars after tokenize, adding to the tokens suffix...\n");
        ASTToken& tok = _state.tokens().global_token();
        tok.suffix_begin_grow( ignored_chars_count );
    }

    TOOLS_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Tokenization.\n%s\n", _state.tokens().to_string().c_str() );

    return true;
}

ASTToken Nodlang::parse_token(const char* buffer, size_t buffer_size, size_t& global_cursor) const
{
    const size_t                  start_pos  = global_cursor;
    const std::string::value_type first_char = buffer[start_pos];
    const size_t                  char_left  = buffer_size - start_pos;

    // comments
    if (first_char == '/' && char_left > 1)
    {
        size_t cursor      = start_pos + 1;
        char   second_char = buffer[cursor];
        if (second_char == '*' || second_char == '/')
        {
            // multi-line comment
            if (second_char == '*')
            {
                while (cursor != buffer_size && !(buffer[cursor] == '/' && buffer[cursor - 1] == '*'))
                {
                    ++cursor;
                }
            }
            // single-line comment
            else
            {
                while (cursor != buffer_size && buffer[cursor] != '\n' )
                {
                    ++cursor;
                }
            }

            ++cursor;
            global_cursor = cursor;
            return ASTToken{ASTToken_t::ignore, const_cast<char*>(buffer), start_pos, cursor - start_pos};
        }
    }

    // single-char
    auto single_char_found = m_token_t_by_single_char.find(first_char);
    if( single_char_found != m_token_t_by_single_char.end() )
    {
        ++global_cursor;
        const ASTToken_t type = single_char_found->second;
        return ASTToken{type, const_cast<char*>(buffer), start_pos, 1};
    }

    // operators
    switch (first_char)
    {
        case '=':
        {
            // "=>" or "=="
            auto cursor = start_pos + 1;
            auto second_char = buffer[cursor];
            if (cursor != buffer_size && (second_char == '>' || second_char == '=')) {
                ++cursor;
                global_cursor = cursor;
                return ASTToken{ASTToken_t::operator_, const_cast<char*>(buffer), start_pos, cursor - start_pos};
            }
            // "="
            global_cursor++;
            return ASTToken{ASTToken_t::operator_, const_cast<char*>(buffer), start_pos, 1};
        }

        case '!':
        case '/':
        case '*':
        case '+':
        case '-':
        case '>':
        case '<':
        {
            // "<operator>=" (do not handle: "++", "--")
            auto cursor = start_pos + 1;
            if (cursor != buffer_size && buffer[cursor] == '=') {
                ++cursor;
                // special case for "<=>" operator
                if (first_char == '<' && cursor != buffer_size && buffer[cursor] == '>') {
                    ++cursor;
                }
                global_cursor = cursor;
            } else {
                // <operator>
                global_cursor++;
            }
            return ASTToken{ASTToken_t::operator_, const_cast<char*>(buffer), start_pos, cursor - start_pos};
        }
    }

    // number (double)
    //     note: we accept zeros as prefix (ex: "0002.15454", or "01012")
    if ( std::isdigit(first_char) )
    {
        auto cursor = start_pos + 1;
        ASTToken_t type = ASTToken_t::literal_int;

        // integer
        while (cursor != buffer_size && std::isdigit(buffer[cursor]))
        {
            ++cursor;
        }

        // double
        if(cursor + 1 < buffer_size
           && buffer[cursor] == '.'      // has a decimal separator
            && std::isdigit(buffer[cursor + 1]) // followed by a digit
           )
        {
            auto local_cursor_decimal_separator = cursor;
            ++cursor;

            // decimal portion
            while (cursor != buffer_size && std::isdigit(buffer[cursor]))
            {
                ++cursor;
            }
            type = ASTToken_t::literal_double;
        }
        global_cursor = cursor;
        return ASTToken{type, const_cast<char*>(buffer), start_pos, cursor - start_pos};
    }

    // double-quoted string
    if (first_char == '"')
    {
        auto cursor = start_pos + 1;
        while (cursor != buffer_size && (buffer[cursor] != '"' || buffer[cursor - 1] == '\\'))
        {
            ++cursor;
        }
        ++cursor;
        global_cursor = cursor;
        return ASTToken{ASTToken_t::literal_string, const_cast<char*>(buffer), start_pos, cursor - start_pos};
    }

    // symbol (identifier or keyword)
    if ( std::isalpha(first_char) || first_char == '_' )
    {
        // parse symbol
        auto cursor = start_pos + 1;
        while (cursor != buffer_size && std::isalnum(buffer[cursor]) || buffer[cursor] == '_' )
        {
            ++cursor;
        }
        global_cursor = cursor;

        ASTToken_t type = ASTToken_t::identifier;

        const auto key = Hash::hash( buffer + start_pos, cursor - start_pos );
        auto keyword_found = m_token_t_by_keyword.find( key );
        if (keyword_found != m_token_t_by_keyword.end())
        {
            // a keyword has priority over identifier
            type = keyword_found->second;
        }
        return ASTToken{type, const_cast<char*>(buffer), start_pos, cursor - start_pos};
    }
    return ASTToken_t::none;
}

ASTNodeSlot* Nodlang::parse_function_call(ASTScope* parent_scope)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "parse function call...\n");

    // Check if the minimum token count required is available ( 0: identifier, 1: open parenthesis, 2: close parenthesis)
    if (!_state.tokens().can_eat(3))
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " 3 tokens min. are required\n");
        return nullptr;
    }

    _state.start_transaction();

    // Try to parse regular function: function(...)
    std::string fct_id;
    ASTToken token_0 = _state.tokens().eat();
    ASTToken token_1 = _state.tokens().eat();
    if (token_0.m_type == ASTToken_t::identifier &&
        token_1.m_type == ASTToken_t::parenthesis_open)
    {
        fct_id = token_0.word_to_string();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Regular function pattern detected.\n");
    }
    else// Try to parse operator like (ex: operator==(..,..))
    {
        ASTToken token_2 = _state.tokens().eat();// eat a "supposed open bracket>

        if (token_0.m_type == ASTToken_t::keyword_operator && token_1.m_type == ASTToken_t::operator_ && token_2.m_type == ASTToken_t::parenthesis_open)
        {
            fct_id = token_1.word_to_string();// operator
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Operator function-like pattern detected.\n");
        }
        else
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Not a function.\n");
            _state.rollback();
            return nullptr;
        }
    }
    std::vector<ASTNodeSlot*> result_slots;

    // Declare a new function prototype
    FunctionDescriptor signature;
    signature.init<any()>(fct_id.c_str());

    bool parsingError = false;
    while (!parsingError && _state.tokens().can_eat() &&
           _state.tokens().peek().m_type != ASTToken_t::parenthesis_close)
    {
        ASTNodeSlot* expression_out = parse_expression(parent_scope);
        if ( expression_out )
        {
            result_slots.push_back( expression_out );
            signature.push_arg( expression_out->property->get_type() );
            _state.tokens().eat_if(ASTToken_t::list_separator);
        }
        else
        {
            parsingError = true;
        }
    }

    // eat "close bracket supposed" token
    if ( !_state.tokens().eat_if(ASTToken_t::parenthesis_close) )
    {
        TOOLS_LOG(tools::Verbosity_Warning, "Parser", TOOLS_KO " Expecting parenthesis close\n");
        _state.rollback();
        return nullptr;
    }


    // Find the prototype in the language library
    ASTFunctionCall* fct_node = _state.graph()->create_function( signature, parent_scope );

    for ( int i = 0; i < fct_node->get_arg_slots().size(); i++ )
    {
        // Connects each results to the corresponding input
        _state.graph()->connect_or_merge(result_slots.at(i), fct_node->get_arg_slot(i) );
    }

    _state.commit();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Function call parsed:\n%s\n", _state.tokens().to_string().c_str() );

    return fct_node->value_out();
}

ASTNode* Nodlang::parse_if_block(ASTScope* parent_scope, ASTNodeSlot* flow_out)
{
    _state.start_transaction();

    ASTToken if_token = _state.tokens().eat_if(ASTToken_t::keyword_if);
    if ( !if_token )
    {
        return nullptr;
    }

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing conditional structure...\n");

    bool    success = false;
    ASTIf*  if_node = _state.graph()->create_cond_struct( parent_scope );
    if_node->token_if  = _state.tokens().get_eaten();

    _state.graph()->connect(flow_out, if_node->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );

    if (_state.tokens().eat_if(ASTToken_t::parenthesis_open) )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing conditional structure's condition...\n");

        // condition
        parse_expression_block(if_node->internal_scope(), nullptr, if_node->condition_in());

        if (_state.tokens().eat_if(ASTToken_t::parenthesis_close) )
        {
            // scope
            ASTNode* block = parse_atomic_code_block( if_node->internal_scope(), if_node->branch_out(Branch_TRUE) );

            if ( block )
            {
                // else
                if ( _state.tokens().eat_if(ASTToken_t::keyword_else) )
                {
                    if_node->token_else = _state.tokens().get_eaten();

                    if ( ASTNode* else_block = parse_atomic_code_block( if_node->internal_scope(), if_node->branch_out(Branch_FALSE) ) )
                    {
                        success = true;
                        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " else block parsed.\n");
                    }
                    else
                    {
                        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Single instruction or root_scope expected\n");
                    }
                }
                else
                {
                    success = true;
                }
            }
            else
            {
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Single instruction or root_scope expected\n");
            }
        }
        else
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Close bracket expected\n");
        }
    }

    if ( success )
    {
        _state.commit();
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Parse conditional structure:\n%s\n", _state.tokens().to_string().c_str() );
        // TODO: connect true/false branches flow_out to scope flow_leave?"
        return if_node;
    }

    _state.graph()->find_and_destroy(if_node);
    _state.rollback();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Parse conditional structure \n");

    return {};
}

ASTNode* Nodlang::parse_for_block(ASTScope* parent_scope, ASTNodeSlot* flow_out)
{
    bool         success  = false;
    ASTForLoop* for_node = nullptr;

    _state.start_transaction();

    if ( ASTToken token_for = _state.tokens().eat_if(ASTToken_t::keyword_for) )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing for loop ...\n");

        for_node = _state.graph()->create_for_loop( parent_scope );
        for_node->token_for = token_for;

        _state.graph()->connect( flow_out, for_node->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );

        ASTToken open_bracket = _state.tokens().eat_if(ASTToken_t::parenthesis_open);
        if ( open_bracket)
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing for set_name/condition/iter instructions ...\n");

            // first we parse three instructions, no matter if we find them, we'll continue (we are parsing something abstract)

            // parse init; condition; iteration or nothing
            parse_expression_block(for_node->internal_scope(), nullptr, for_node->initialization_slot())
            && parse_expression_block(for_node->internal_scope(), nullptr, for_node->condition_in())
            && parse_expression_block(for_node->internal_scope(), nullptr, for_node->iteration_slot());

            // parse parenthesis close
            if ( ASTToken parenthesis_close = _state.tokens().eat_if(ASTToken_t::parenthesis_close) )
            {
                ASTNode* block = parse_atomic_code_block( for_node->internal_scope(), for_node->branch_out(Branch_TRUE) ) ;

                if ( block )
                {
                    success = true;
                    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Scope or single instruction found\n");
                }
                else
                {
                    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Scope or single instruction expected\n");
                }
            }
            else
            {
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Close parenthesis was expected.\n");
            }
        }
        else
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Open parenthesis was expected.\n");
        }
    }

    if ( success )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " For block parsed\n");
        _state.commit();
        // TODO: Should we connect true/false branches to scope's flow_leave Slot?
        return for_node;
    }

    if ( for_node )
    {
        _state.graph()->find_and_destroy(for_node);
    }
    _state.rollback();
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " Could not parse for block\n");
    return {};
}

ASTNode* Nodlang::parse_while_block(ASTScope* parent_scope,  ASTNodeSlot* flow_out)
{
    bool           success    = false;
    ASTWhileLoop*  while_node = nullptr;
    ASTNode*       block      = nullptr;

    _state.start_transaction();

    if ( ASTToken token_while = _state.tokens().eat_if(ASTToken_t::keyword_while) )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing while ...\n");

        while_node = _state.graph()->create_while_loop( parent_scope );
        while_node->token_while = token_while;

        _state.graph()->connect( flow_out, while_node->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS );

        if ( ASTToken open_bracket = _state.tokens().eat_if(ASTToken_t::parenthesis_open) )
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing while condition ... \n");

            // Parse an optional condition
            parse_expression_block(while_node->internal_scope(), nullptr, while_node->condition_in());

            if (_state.tokens().eat_if(ASTToken_t::parenthesis_close) )
            {
                block = parse_atomic_code_block( while_node->internal_scope(), while_node->branch_out(Branch_TRUE) );
                if ( block )
                {
                    success = true;
                }
                else
                {
                    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO "  Scope or single instruction expected\n");
                }
            }
            else
            {
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO "  Parenthesis close expected\n");
            }
        }
        else
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO "  Parenthesis close expected\n");
        }
    }

    if ( success )
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing while:\n%s\n", _state.tokens().to_string().c_str() );
        _state.commit();
        // TODO: Should we connect true/false branches to scope's flow_leave SLot?
        return while_node;
    }

    _state.rollback();
    _state.graph()->find_and_destroy(while_node);
    _state.graph()->find_and_destroy(block);

    return {};
}

ASTNodeSlot* Nodlang::parse_variable_declaration(ASTScope* parent_scope)
{
    if (!_state.tokens().can_eat(2))
    {
        return nullptr;
    }

    _state.start_transaction();

    bool  success          = false;
    ASTToken type_token       = _state.tokens().eat();
    ASTToken identifier_token = _state.tokens().eat();

    if (type_token.is_keyword_type() && identifier_token.m_type == ASTToken_t::identifier)
    {
        const TypeDescriptor* type = get_type(type_token.m_type);
        ASTVariable* variable_node = _state.graph()->create_variable( type, identifier_token.word_to_string(), parent_scope );
        variable_node->set_flags(VariableFlag_DECLARED);
        variable_node->set_type_token( type_token );
        variable_node->set_identifier_token( identifier_token );

        // declaration with assignment ?
        ASTToken operator_token = _state.tokens().eat_if(ASTToken_t::operator_);
        if (operator_token && operator_token.word_len() == 1 && *operator_token.word() == '=')
        {
            // an expression is expected
            if ( ASTNodeSlot* expression_out = parse_expression(parent_scope) )
            {
                // expression's out ----> variable's in
                _state.graph()->connect_to_variable(expression_out, variable_node );

                variable_node->set_operator_token( operator_token );
                success = true;
            }
            else
            {
                TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO "  Initialization expression expected for %s\n", identifier_token.word_to_string().c_str());
            }
        }
            // Declaration without assignment
        else
        {
            success = true;
        }

        if ( success )
        {
            TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Variable declaration: %s %s\n",
                        variable_node->value()->get_type()->name(),
                        identifier_token.word_to_string().c_str());
            _state.commit();
            return variable_node->value_out();
        }

        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO "  Initialization expression expected for %s\n", identifier_token.word_to_string().c_str());
        _state.graph()->find_and_destroy(variable_node);
    }

    _state.rollback();
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------------
// [SECTION] C. Serializer --------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

const ASTNodeSlot* Nodlang::serialize_invokable(std::string &_out, const ASTFunctionCall* _node) const
{
    if (_node->type() == ASTNodeType_OPERATOR )
    {
        const std::vector<ASTNodeSlot*>& args = _node->get_arg_slots();
        int precedence = get_precedence(&_node->get_func_type());

        switch ( _node->get_func_type().arg_count() )
        {
            case 2:
            {
                // Left part of the expression
                {
                    const FunctionDescriptor* l_func_type = _node->get_connected_function_type(LEFT_VALUE_PROPERTY);
                    bool needs_braces = l_func_type && get_precedence(l_func_type) < precedence;
                    SerializeFlags flags = SerializeFlag_RECURSE
                                         | needs_braces * SerializeFlag_WRAP_WITH_BRACES ;
                    serialize_input( _out, args[0], flags );
                }

                // Operator
                VERIFY( _node->get_identifier_token(), "identifier token should have been assigned in parse_function_call");
                serialize_token( _out, _node->get_identifier_token() );

                // Right part of the expression
                {
                    const FunctionDescriptor* r_func_type = _node->get_connected_function_type(RIGHT_VALUE_PROPERTY);
                    bool needs_braces = r_func_type && get_precedence(r_func_type) < precedence;
                    SerializeFlags flags = SerializeFlag_RECURSE
                                         | needs_braces * SerializeFlag_WRAP_WITH_BRACES ;
                    serialize_input( _out, args[1], flags );
                }
                break;
            }

            case 1:
            {
                // operator ( ... innerOperator ... )   ex:   -(a+b)

                ASSERT( _node->get_identifier_token() );
                serialize_token(_out, _node->get_identifier_token());

                bool needs_braces    = _node->get_connected_function_type(LEFT_VALUE_PROPERTY) != nullptr;
                SerializeFlags flags = SerializeFlag_RECURSE
                                     | needs_braces * SerializeFlag_WRAP_WITH_BRACES;
                serialize_input( _out, args[0], flags );
                break;
            }
        }
    }
    else
    {
        serialize_func_call(_out, &_node->get_func_type(), _node->get_arg_slots());
    }

    return _node->value_out();
}

std::string &Nodlang::serialize_func_call(std::string &_out, const FunctionDescriptor *_signature, const std::vector<ASTNodeSlot*> &inputs) const
{
    _out.append( _signature->get_identifier() );
    serialize_default_buffer(_out, ASTToken_t::parenthesis_open);

    for (const ASTNodeSlot* input_slot : inputs)
    {
        ASSERT( input_slot->has_flags(SlotFlag_INPUT) );
        if ( input_slot != inputs.front())
        {
            serialize_default_buffer(_out, ASTToken_t::list_separator);
        }
        serialize_input( _out, input_slot, SerializeFlag_RECURSE );
    }

    serialize_default_buffer(_out, ASTToken_t::parenthesis_close);
    return _out;
}

std::string &Nodlang::serialize_invokable_sig(std::string &_out, const IInvokable* _invokable) const
{
    return serialize_func_sig(_out, _invokable->get_sig());
}

std::string &Nodlang::serialize_func_sig(std::string &_out, const FunctionDescriptor *_signature) const
{
    serialize_type(_out, _signature->return_type());
    _out.append(" ");
    _out.append(_signature->get_identifier());
    serialize_default_buffer(_out, ASTToken_t::parenthesis_open);

    auto args = _signature->arg();
    for (auto it = args.begin(); it != args.end(); it++)
    {
        if (it != args.begin())
        {
            serialize_default_buffer(_out, ASTToken_t::list_separator);
            _out.append(" ");
        }
        serialize_type(_out, it->type);
    }

    serialize_default_buffer(_out, ASTToken_t::parenthesis_close);
    return _out;
}

std::string &Nodlang::serialize_type(std::string &_out, const TypeDescriptor *_type) const
{
    auto found = m_keyword_by_type_id.find(_type->id());
    if (found != m_keyword_by_type_id.cend())
    {
        return _out.append(found->second);
    }
    return _out;
}

std::string& Nodlang::serialize_variable_ref(std::string &_out, const ASTVariableRef* _node) const
{
    return serialize_token( _out, _node->get_identifier_token() );
}

std::string& Nodlang::serialize_variable(std::string &_out, const ASTVariable *_node) const
{
    // 1. Serialize variable's type

    // If parsed
    if ( _node->get_type_token() )
    {
        serialize_token(_out, _node->get_type_token());
    }
    else // If created in the graph by the user
    {
        serialize_type(_out, _node->value()->get_type());
        _out.append(" ");
    }

    // 2. Serialize variable identifier
    serialize_token( _out, _node->get_identifier_token() );

    // 3. Initialisation
    //    When a VariableNode has its input connected, we serialize it as its initialisation expression

    const ASTNodeSlot* slot = _node->value_in();
    if ( slot->adjacent_count() != 0 )
    {
        if ( _node->get_operator_token() )
            _out.append(_node->get_operator_token().string());
        else
            _out.append(" = ");

        serialize_input( _out, slot, SerializeFlag_RECURSE );
    }
    return _out;
}

std::string &Nodlang::serialize_input(std::string& _out, const ASTNodeSlot* slot, SerializeFlags _flags ) const
{
    ASSERT( slot->has_flags( SlotFlag_INPUT ) );

    const ASTNodeSlot*     adjacent_slot     = slot->first_adjacent();
    const ASTNodeProperty* adjacent_property = adjacent_slot != nullptr ? adjacent_slot->property
                                                                        : nullptr;
    // Append open brace?
    if ( _flags & SerializeFlag_WRAP_WITH_BRACES )
        serialize_default_buffer(_out, ASTToken_t::parenthesis_open);

    if ( adjacent_property == nullptr )
    {
        // Simply serialize this property
        serialize_property(_out, slot->property);
    }
    else
    {
        VERIFY( _flags & SerializeFlag_RECURSE, "Why would you call serialize_input without RECURSE flag?");
        // Append token prefix?
        if (const ASTToken& adjacent_token = adjacent_property->token())
            if ( adjacent_token )
                _out.append(adjacent_token.prefix(), adjacent_token.prefix_len() );

        // Serialize adjacent slot
        serialize_value_out(_out, adjacent_slot, SerializeFlag_RECURSE);

        // Append token suffix?
        if (const ASTToken& adjacent_token = adjacent_property->token())
            if ( adjacent_token )
                _out.append(adjacent_token.suffix(), adjacent_token.suffix_len() );
    }

    // Append close brace?
    if ( _flags & SerializeFlag_WRAP_WITH_BRACES )
        serialize_default_buffer(_out, ASTToken_t::parenthesis_close);

    return _out;
}

std::string &Nodlang::serialize_value_out(std::string& _out, const ASTNodeSlot* slot, SerializeFlags _flags) const
{
    // If output is node's output value, we serialize the node
    if( slot == slot->node->value_out() )
    {
        serialize_node(_out, slot->node, _flags);
        return _out;
    }

    // Otherwise, it might be a variable reference, so we serialize the identifier only
    ASSERT(slot->node->type() == ASTNodeType_VARIABLE ); // Can't be another type
    auto variable = static_cast<const ASTVariable*>( slot->node );
    VERIFY( slot == variable->ref_out(), "Cannot serialize an other slot from a VariableNode");
    return _out.append( variable->get_identifier() );
}

std::string& Nodlang::serialize_node(std::string &_out, const ASTNode* node, SerializeFlags _flags ) const
{
    if ( node == nullptr )
        return _out;

    ASSERT( _flags == SerializeFlag_RECURSE ); // The only flag configuration handled for now

    switch ( node->type() )
    {
        case ASTNodeType_IF_ELSE:
            serialize_cond_struct(_out, static_cast<const ASTIf*>(node) );
            break;
        case ASTNodeType_FOR_LOOP:
            serialize_for_loop(_out, static_cast<const ASTForLoop*>(node) );
            break;
        case ASTNodeType_WHILE_LOOP:
            serialize_while_loop(_out, static_cast<const ASTWhileLoop*>(node) );
            break;
        case ASTNodeType_LITERAL:
            serialize_literal(_out, static_cast<const ASTLiteral*>(node) );
            break;
        case ASTNodeType_VARIABLE:
            serialize_variable(_out, static_cast<const ASTVariable*>(node));
            break;
        case ASTNodeType_VARIABLE_REF:
            serialize_variable_ref(_out, static_cast<const ASTVariableRef*>(node));
            break;
        case ASTNodeType_FUNCTION:
            [[fallthrough]];
        case ASTNodeType_OPERATOR:
            serialize_invokable(_out, static_cast<const ASTFunctionCall*>(node) );
            break;
        case ASTNodeType_EMPTY_INSTRUCTION:
            serialize_empty_instruction(_out, node);
            break;
        case ASTNodeType_SCOPE:
            serialize_scope(_out, node->internal_scope() );
            break;
        default:
            VERIFY(false, "Unhandled NodeType, can't serialize");
    }
    serialize_token(_out, node->suffix() );

    return _out;
}

std::string& Nodlang::serialize_scope(std::string &_out, const ASTScope* scope) const
{
    serialize_token(_out, scope->token_begin);
    for(ASTNode* node : scope->backbone() )
    {
        serialize_node(_out, node, SerializeFlag_RECURSE);
    }
    serialize_token(_out, scope->token_end);

    return _out;
}

std::string &Nodlang::serialize_token(std::string& _out, const ASTToken& _token) const
{
    // Skip a null token
    if ( !_token )
        return _out;

    return _out.append(_token.begin(), _token.length());
}

std::string& Nodlang::serialize_graph(std::string &_out, const Graph* graph ) const
{
    if ( !graph->root_node() )
    {
        TOOLS_LOG(tools::Verbosity_Error, "Serializer", "a root primary_child is expected to serialize the graph\n");
        return _out;
    }
    return serialize_node(_out, graph->root_node(), SerializeFlag_RECURSE);
}

std::string& Nodlang::serialize_bool(std::string& _out, bool b) const
{
    return _out.append( b ? "true" : "false");
}

std::string& Nodlang::serialize_int(std::string& _out, int i) const
{
    return _out.append( std::to_string(i) );
}

std::string& Nodlang::serialize_double(std::string& _out, double d) const
{
    return _out.append( format::number(d) );
}

std::string& Nodlang::serialize_for_loop(std::string &_out, const ASTForLoop *_for_loop) const
{
    serialize_token(_out, _for_loop->token_for);
    serialize_default_buffer(_out, ASTToken_t::parenthesis_open);
    {
        const ASTNodeSlot* init_slot = _for_loop->find_slot_by_property_name(INITIALIZATION_PROPERTY, SlotFlag_INPUT );
        const ASTNodeSlot* cond_slot = _for_loop->find_slot_by_property_name(CONDITION_PROPERTY, SlotFlag_INPUT );
        const ASTNodeSlot* iter_slot = _for_loop->find_slot_by_property_name(ITERATION_PROPERTY, SlotFlag_INPUT );
        serialize_input( _out, init_slot, SerializeFlag_RECURSE );
        serialize_input( _out, cond_slot, SerializeFlag_RECURSE );
        serialize_input( _out, iter_slot, SerializeFlag_RECURSE );
    }
    serialize_default_buffer(_out, ASTToken_t::parenthesis_close);
    serialize_node(_out, _for_loop->branch_out(Branch_TRUE)->first_adjacent_node(), SerializeFlag_RECURSE );

    return _out;
}

std::string& Nodlang::serialize_while_loop(std::string &_out, const ASTWhileLoop *_while_loop_node) const
{
    // while
    serialize_token(_out, _while_loop_node->token_while);

    // condition
    SerializeFlags flags = SerializeFlag_RECURSE
                         | SerializeFlag_WRAP_WITH_BRACES;
    serialize_input(_out, _while_loop_node->condition_in(), flags );

    if ( const ASTNode* _node = _while_loop_node->branch_out(Branch_TRUE)->first_adjacent_node() )
    {
        serialize_node(_out, _node, SerializeFlag_RECURSE);
    }

    return _out;
}


std::string& Nodlang::serialize_cond_struct(std::string &_out, const ASTIf* if_node ) const
{
    // if
    serialize_token(_out, if_node->token_if);

    // condition
    SerializeFlags flags = SerializeFlag_RECURSE
                         | SerializeFlag_WRAP_WITH_BRACES;
    serialize_input(_out, if_node->condition_in(), flags );

    // when condition is true
    serialize_node(_out, if_node->branch_out(Branch_TRUE)->first_adjacent_node(), SerializeFlag_RECURSE );

    // when condition is false
    serialize_token(_out, if_node->token_else);
    serialize_node(_out, if_node->branch_out(Branch_FALSE)->first_adjacent_node(), SerializeFlag_RECURSE );

    return _out;
}

// Language definition ------------------------------------------------------------------------------------------------------------

std::string& Nodlang::serialize_property(std::string& _out, const ASTNodeProperty* _property) const
{
    return serialize_token(_out, _property->token());
}

const Operator *Nodlang::find_operator(const std::string &_identifier, Operator_t operator_type) const
{
    auto is_exactly = [&](const Operator *op) {
        return op->identifier == _identifier && op->type == operator_type;
    };

    auto found = std::find_if(m_operators.cbegin(), m_operators.cend(), is_exactly);

    if (found != m_operators.end())
        return *found;

    return nullptr;
}

bool Nodlang::is_operator(const FunctionDescriptor* descriptor) const
{
    switch ( descriptor->arg_count() )
    {
        case 1:
            return find_operator( descriptor->name(), tools::Operator_t::Unary );
        case 2:
            return find_operator( descriptor->name(), tools::Operator_t::Binary );
        default:
            return false;
    }
}

std::string& Nodlang::serialize_default_buffer(std::string& _out, ASTToken_t _token_t) const
{
    switch (_token_t)
    {
        case ASTToken_t::end_of_line:     return _out.append("\n"); // TODO: handle all platforms
        case ASTToken_t::operator_:       return _out.append("operator");
        case ASTToken_t::identifier:      return _out.append("identifier");
        case ASTToken_t::literal_string:  return _out.append("\"\"");
        case ASTToken_t::literal_double:  return _out.append("0.0");
        case ASTToken_t::literal_int:     return _out.append("0");
        case ASTToken_t::literal_bool:    return _out.append("false");
        case ASTToken_t::literal_any:     return _out.append("0");
        case ASTToken_t::ignore:          [[fallthrough]];
        case ASTToken_t::literal_unknown: return _out;
        default:
        {
            {
                auto found = m_keyword_by_token_t.find(_token_t);
                if (found != m_keyword_by_token_t.cend())
                {
                    return _out.append(found->second);
                }
            }
            {
                auto found = m_single_char_by_keyword.find(_token_t);
                if (found != m_single_char_by_keyword.cend())
                {
                    _out.push_back(found->second);
                    return _out;
                }
            }
            return _out.append("<?>");
        }
    }
}

std::string Nodlang::serialize_type(const TypeDescriptor *_type) const
{
    std::string result;
    serialize_type(result, _type);
    return result;
}

int Nodlang::get_precedence( const tools::FunctionDescriptor* _func_type) const
{
    if (!_func_type)
        return std::numeric_limits<int>::min(); // default

    const Operator* operator_ptr = find_operator(_func_type->get_identifier(), static_cast<Operator_t>(_func_type->arg_count()));

    if (operator_ptr)
        return operator_ptr->precedence;
    return std::numeric_limits<int>::max();
}

const TypeDescriptor* Nodlang::get_type(ASTToken_t _token) const
{
    auto found = m_type_by_token_t.find(_token);
    if ( found != m_type_by_token_t.end() )
        return found->second;
    return nullptr;
}

ASTToken Nodlang::parse_token(const std::string &_string) const
{
    size_t cursor = 0;
    return parse_token( const_cast<char*>(_string.data()), _string.length(), cursor);
}

bool Nodlang::accepts_suffix(ASTToken_t type) const
{
    return type != ASTToken_t::identifier          // identifiers must stay clean because they are reused
              && type != ASTToken_t::parenthesis_open    // ")" are lost when creating AST
              && type != ASTToken_t::parenthesis_close;  // "(" are lost when creating AST
}

ASTToken_t Nodlang::to_literal_token(const TypeDescriptor *type) const
{
    if (type == type::get<double>() )
        return ASTToken_t::literal_double;
    if (type == type::get<i16_t>() )
        return ASTToken_t::literal_int;
    if (type == type::get<int>() )
        return ASTToken_t::literal_int;
    if (type == type::get<bool>() )
        return ASTToken_t::literal_bool;
    if (type == type::get<std::string>() )
        return ASTToken_t::literal_string;
    if (type == type::get<any>() )
        return ASTToken_t::literal_any;
    return ASTToken_t::literal_unknown;
}

ASTNode* Nodlang::parse_atomic_code_block(ASTScope* parent_scope, ASTNodeSlot* flow_out)
{
    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", "Parsing atomic code block ..\n");
    ASSERT(flow_out);

    // most common case
    ASTNode* block = nullptr;
         if ( (block = parse_scoped_block(parent_scope, flow_out)) );
    else if ( (block = parse_expression_block(parent_scope, flow_out)) );
    else if ( (block = parse_if_block(parent_scope, flow_out)) );
    else if ( (block = parse_for_block(parent_scope, flow_out)) );
    else if ( (block = parse_while_block(parent_scope, flow_out)) ) ;
    else      (block = parse_empty_block(parent_scope, flow_out));

    if ( block )
    {
        if ( ASTToken tok = _state.tokens().eat_if(ASTToken_t::end_of_instruction) )
        {
            block->set_suffix(tok );
        }

        TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_OK " Block found (class %s)\n", block->get_class()->name() );
        return block;
    }

    TOOLS_DEBUG_LOG(tools::Verbosity_Diagnostic, "Parser", TOOLS_KO " No block found\n");
    return nullptr;
}

std::string& Nodlang::serialize_literal(std::string &_out, const ASTLiteral* node) const
{
    return serialize_property( _out, node->value() );
}

std::string& Nodlang::serialize_empty_instruction(std::string &_out, const ASTNode* node) const
{
    return serialize_token(_out, node->value()->token() );
}

ASTNode* Nodlang::parse_empty_block(ASTScope* parent_scope, ASTNodeSlot* flow_out)
{
    if ( _state.tokens().peek(ASTToken_t::end_of_instruction) )
    {
        ASTNode* node = _state.graph()->create_empty_instruction( parent_scope );
        _state.graph()->connect( flow_out, node->flow_in(), GraphFlag_ALLOW_SIDE_EFFECTS);
        return node;
    }
    return nullptr;
}

void Nodlang::ParserState::reset_graph(Graph* new_graph)
{
    new_graph->reset();
    _graph = new_graph; // memory not owned
}

void Nodlang::ParserState::reset_ribbon(const char* new_buf, size_t new_size)
{
    ASSERT( new_size == 0 || new_buf != nullptr);
    _buffer = { new_buf, new_size };
    _ribbon.reset( new_buf, new_size );
}

Nodlang* ndbl::init_language()
{
    ASSERT(g_language == nullptr);
    g_language = new Nodlang();
    return g_language;
}

Nodlang* ndbl::get_language()
{
    VERIFY(g_language, "No language found, did you call init_language?");
    return g_language;
}

void ndbl::shutdown_language(Nodlang* _language)
{
    ASSERT(g_language == _language); // singleton for now
    ASSERT(g_language != nullptr);
    delete g_language;
    g_language = nullptr;
}

