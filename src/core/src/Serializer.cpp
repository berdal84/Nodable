#include <nodable/core/Serializer.h>
#include <nodable/core/Member.h>
#include <nodable/core/InvokableComponent.h>
#include <nodable/core/GraphNode.h>
#include <nodable/core/VariableNode.h>
#include <nodable/core/InstructionNode.h>
#include <nodable/core/ConditionalStructNode.h>
#include <nodable/core/ForLoopNode.h>
#include <nodable/core/Scope.h>
#include <nodable/core/LiteralNode.h>

using namespace Nodable;

std::string& Serializer::serialize(std::string& _result, const InvokableComponent *_component)const
{
    const Signature* signature = _component->get_signature();

    if ( !signature->is_operator() )
    {
        serialize(_result, signature, _component->get_args());
    }
    else
    {
        // generic serialize member lambda
        auto serialize_member_with_or_without_brackets = [this, &_result](Member* member, bool needs_brackets)
        {
            if (needs_brackets)
            {
                serialize(_result, Token_t::open_bracket);
            }

            serialize(_result, member);

            if (needs_brackets)
            {
                serialize(_result, Token_t::close_bracket);
            }
        };

        Node*            owner = _component->get_owner();
        const MemberVec& args  = _component->get_args();
        const Signature*   sig = signature;

        switch (sig->get_arg_count())
        {
            case 2:
            {
                // Get the left and right source operator
                auto l_handed_operator = owner->get_connected_operator(args[0]);
                auto r_handed_operator = owner->get_connected_operator(args[1]);

                // Left part of the expression
                {
                    // TODO: check parsed brackets for prefix/suffix
                    bool needs_brackets = l_handed_operator &&
                            l_handed_operator->get_signature()->get_operator()->precedence < sig->get_operator()->precedence;

                    serialize_member_with_or_without_brackets(args[0], needs_brackets);
                }

                // Operator
                std::shared_ptr<Token> sourceToken = _component->get_source_token();
                if (sourceToken)
                {
                    _result.append(sourceToken->m_prefix);
                    _result.append(sourceToken->m_word);
                    _result.append(sourceToken->m_suffix);
                }
                else
                {
                    _result.append(sig->get_operator()->identifier);
                }

                // Right part of the expression
                {
                    // TODO: check parsed brackets for prefix/suffix
                    bool needs_brackets = r_handed_operator && (r_handed_operator->get_signature()->get_arg_count() == 1
                                         || r_handed_operator->get_signature()->get_operator()->precedence < sig->get_operator()->precedence );

                    serialize_member_with_or_without_brackets(args[1], needs_brackets);
                }
                break;
            }

            case 1:
            {
                // operator ( ... innerOperator ... )   ex:   -(a+b)

                // Operator
                std::shared_ptr<Token> token = _component->get_source_token();

                if (token) _result.append(token->m_prefix);

                _result.append(sig->get_operator()->identifier);

                if (token) _result.append(token->m_suffix);

                auto inner_operator = owner->get_connected_operator(args[0]);
                serialize_member_with_or_without_brackets(args[0], inner_operator != nullptr);
                break;
            }
        }
    }
    return _result;
}

std::string& Serializer::serialize(std::string& _result, const Signature*   _signature, const std::vector<Member*>& _args) const
{
    _result.append(_signature->get_identifier());
    serialize(_result, Token_t::open_bracket);

    for (auto it = _args.begin(); it != _args.end(); it++)
    {
        serialize(_result, *it);

        if (*it != _args.back())
        {
            serialize(_result, Token_t::separator);
        }
    }

    serialize(_result, Token_t::close_bracket);
    return _result;
}

std::string& Serializer::serialize(std::string& _result, const Signature* _signature) const
{
    serialize(_result, _signature->get_return_type());
    _result.append(" ");
    _result.append(_signature->get_identifier() );
    serialize(_result, Token_t::open_bracket);

    auto args = _signature->get_args();
    for (auto it = args.begin(); it != args.end(); it++)
    {
        if (it != args.begin())
        {
            serialize( _result, Token_t::separator);
            _result.append(" ");
        }
        serialize(_result, it->m_type);
    }

    serialize(_result, Token_t::close_bracket );
    return  _result;
}

std::string& Serializer::serialize(std::string& _result, const Token_t& _type) const
{
    return _result.append(language->get_semantic()->token_type_to_string(_type) );
}

std::string& Serializer::serialize(std::string &_result, std::shared_ptr<const R::MetaType> _type) const
{
    return _result.append(language->get_semantic()->type_to_string(_type) );
}

std::string& Serializer::serialize(std::string& _result, const VariableNode* _node) const
{
    const InstructionNode* decl_instr = _node->get_declaration_instr();

    // type
    if ( decl_instr )
    {
        if ( std::shared_ptr<const Token> type_tok = _node->get_type_token() )
        {
            serialize(_result, type_tok );
        }
        else // in case no token found (means was not parsed but created by the user)
        {
            serialize(_result, _node->get_value()->get_meta_type() );
            _result.append(" ");
        }
    }

    // var name
    std::shared_ptr<const Token> identifier_token = _node->get_identifier_token();
    if ( identifier_token ) _result.append(identifier_token->m_prefix);
    _result.append(_node->get_name());
    if ( identifier_token ) _result.append(identifier_token->m_suffix);

    Member* value = _node->get_value();

    if( decl_instr )
    {
        auto append_assign_tok  = [&]()
        {
            std::shared_ptr<const Token> assign_tok = _node->get_assignment_operator_token();
            if (assign_tok )
            {
                _result.append(assign_tok->m_prefix );
                _result.append(assign_tok->m_word ); // is "="
                _result.append(assign_tok->m_suffix );
            }
            else
            {
                _result.append(" = ");
            }
        };

        if (value->has_input_connected() )
        {
            append_assign_tok();
            serialize(_result, value);
        }
        else if ( value->get_data()->is_defined() )
        {
            append_assign_tok();
            _result.append(value->get_src_token()->m_prefix);
            serialize(_result, value->get_data());
            _result.append(value->get_src_token()->m_suffix);
        }
    }
    return _result;
}

std::string& Serializer::serialize(std::string& _result, const Variant* variant) const
{
    if (variant->get_meta_type()->get_type() == R::Type::string_t )
    {
        return _result.append('"' + variant->convert_to<std::string>() + '"');
    }
    else
    {
        return _result.append(variant->convert_to<std::string>() );
    }
}

std::string& Serializer::serialize(std::string& _result, const Member * _member, bool followConnections) const
{
    // specific case of a Node*
    if (_member->get_meta_type()->is_exactly( R::get_meta_type<Node*>()))
    {
        if(_member->get_data()->is_initialized())
        {
            return serialize(_result, (const Node*)*_member);
        }
    }

    std::shared_ptr<Token> sourceToken = _member->get_src_token();
    if (sourceToken)
    {
        _result.append(sourceToken->m_prefix);
    }

    auto owner = _member->get_owner();
    if (followConnections && owner && _member->allows_connection(Way_In) && owner->has_wire_connected_to(_member) )
    {
        Member* src_member = _member->get_input();
        InvokableComponent* compute_component = src_member->get_owner()->get<InvokableComponent>();

        if ( compute_component )
        {
            serialize(_result, compute_component );
        }
        else
        {
            serialize(_result, src_member, false);
        }
    }
    else
    {
        if (owner && owner->get_class() == VariableNode::Get_class())
        {
            auto variable = owner->as<VariableNode>();
            _result.append(variable->get_name() );
        }
        else
        {
            serialize(_result, _member->get_data() );
        }
    }

    if (sourceToken)
    {
        _result.append(sourceToken->m_suffix);
    }
    return _result;
}

std::string& Serializer::serialize(std::string& _result, const Node* _node) const
{
    NODABLE_ASSERT(_node != nullptr)
    auto clss = _node->get_class();

    if (clss->is_child_of<InstructionNode>())
    {
        serialize(_result, _node->as<InstructionNode>());
    }
    else if (clss->is_child_of<ConditionalStructNode>() )
    {
        serialize( _result, _node->as<ConditionalStructNode>());
    }
    else if (clss->is_child_of<ForLoopNode>() )
    {
        serialize( _result, _node->as<ForLoopNode>());
    }
    else if ( _node->has<Scope>() )
    {
        serialize( _result, _node->get<Scope>() );
    }
    else if ( _node->is<LiteralNode>() )
    {
        serialize( _result, _node->as<LiteralNode>()->get_value() );
    }
    else if ( _node->is<VariableNode>() )
    {
        serialize( _result, _node->as<VariableNode>() );
    }
    else if ( _node->has<InvokableComponent>() )
    {
        serialize( _result, _node->get<InvokableComponent>() );
    }
    else
    {
        std::string message = "Unable to serialize ";
        message.append( _node->get_class()->get_name() );
        throw std::runtime_error( message );
    }

    return _result;
}

std::string& Serializer::serialize(std::string& _result, const Scope* _scope) const
{

    serialize(_result, _scope->get_begin_scope_token() );
    auto& children = _scope->get_owner()->children_slots();
    if (!children.empty())
    {
        for( auto& eachChild : children )
        {
            serialize( _result, eachChild );
        }
    }

    serialize(_result, _scope->get_end_scope_token() );

    return _result;
}

std::string& Serializer::serialize(std::string& _result, const InstructionNode* _instruction ) const
{
    const Member* root_node_member = _instruction->get_root_node_member();

    if (root_node_member->has_input_connected() && root_node_member->get_data()->is_initialized() )
    {
        auto root_node = (const Node*)*root_node_member;
        NODABLE_ASSERT ( root_node )
        serialize( _result, root_node );
    }

    return serialize( _result, _instruction->end_of_instr_token() );
}

std::string& Serializer::serialize(std::string& _result, std::shared_ptr<const Token> _token)const
{
    if ( _token )
    {
        _result.append( _token->m_prefix);
        if ( _token->m_type == Token_t::unknown )
        {
            _result.append( _token->m_word );
        }
        else
        {
            serialize( _result, _token->m_type );
        }
        _result.append( _token->m_suffix);

    }
    return _result;
}

std::string& Serializer::serialize(std::string& _result, const ForLoopNode* _for_loop)const
{

    serialize( _result, _for_loop->get_token_for() );
    serialize( _result, Token_t::open_bracket );

    // TODO: I don't like this if/else, should be implicit. Serialize Member* must do it.
    //       More work to do to know if expression is a declaration or not.

    Member* input = _for_loop->get_init_expr()->get_input();
    if ( input && input->get_owner()->get_class()->is_child_of<VariableNode>() )
    {
        serialize( _result, input->get_owner()->as<VariableNode>() );
    }
    else
    {
        serialize( _result, _for_loop->get_init_instr() );
    }
    serialize( _result, _for_loop->get_cond_instr() );
    serialize( _result, _for_loop->get_iter_instr() );
    serialize( _result, Token_t::close_bracket );

    // if scope
    if ( auto* scope = _for_loop->get_condition_true_scope() )
    {
        serialize( _result, scope );
    }

    return _result;
}

std::string& Serializer::serialize(std::string& _result, const ConditionalStructNode* _condStruct)const
{
    // if ( <condition> )
    serialize( _result, _condStruct->get_token_if() );
    serialize( _result, Token_t::open_bracket );
    serialize( _result, _condStruct->get_cond_instr() );
    serialize( _result, Token_t::close_bracket );

    // if scope
    if ( auto* ifScope = _condStruct->get_condition_true_scope() )
        serialize( _result, ifScope );

    // else & else scope
    if ( std::shared_ptr<const Token> tokenElse = _condStruct->get_token_else() )
    {
        serialize( _result, tokenElse );
        Scope* elseScope = _condStruct->get_condition_false_scope();
        if ( elseScope )
        {
            serialize( _result, elseScope->get_owner() );
        }
    }
    return _result;
}
