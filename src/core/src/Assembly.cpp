#include <nodable/Assembly.h>
#include <nodable/VariableNode.h>
#include <nodable/Log.h>
#include <nodable/AbstractConditionalStruct.h>
#include <nodable/ForLoopNode.h>
#include <nodable/Scope.h>

using namespace Nodable;
using namespace Nodable::Asm;

std::string Asm::Instr::to_string(const Instr& _instr)
{
    std::string result;

    // append "<line> :"
    std::string str = std::to_string(_instr.m_line);
    while( str.length() < 4 )
        str.append(" ");
    result.append( str );
    result.append( ": " );

    // append instruction type
    result.append( Nodable::to_string(_instr.m_type) + " " );

    // optionally append parameters
    switch ( _instr.m_type )
    {
        case Instr_t::call:
        {
            FctId fct_id   = (FctId)_instr.m_left_h_arg;
            Member* member = (Member*)_instr.m_right_h_arg;
            result.append( Nodable::to_string(fct_id) );
            result.append( " [" + std::to_string((size_t)member) + "]");
            break;
        }

        case Instr_t::mov:
        case Instr_t::cmp:
        {
            result.append("%" + std::to_string( _instr.m_left_h_arg ) );
            result.append(", %" + std::to_string( _instr.m_right_h_arg ) );
            break;
        }

        case Instr_t::jne:
        case Instr_t::jmp:
        {
            result.append( std::to_string( _instr.m_left_h_arg ) );
            break;
        }

        case Instr_t::ret: // nothing else to do.
            break;
    }

    // optionally append comment
    if ( !_instr.m_comment.empty() )
    {
        while( result.length() < 50 ) // align on 80th char
            result.append(" ");
        result.append( "; " );
        result.append( _instr.m_comment );
    }
    return result;
}

std::string Nodable::to_string(Instr_t _type)
{
    switch( _type)
    {
        case Instr_t::mov:   return "mov";
        case Instr_t::ret:   return "ret";
        case Instr_t::call:  return "call";
        case Instr_t::jmp:   return "jpm";
        case Instr_t::jne:   return "jne";
        default:             return "???";
    }
}

std::string Nodable::to_string(Register _register)
{
    switch( _register)
    {
        case Register::rax: return "%rax";
        case Register::rdx: return "%rdx";
        default:            return "%???";
    }
}

std::string Nodable::to_string(FctId _id)
{
    switch( _id)
    {
        case FctId::eval_member: return "eval_member";
        default:                 return "???";
    }
}

Code::~Code()
{
    for( auto each : m_instructions )
        delete each;
    m_instructions.clear();
}

Instr* Code::push_instr(Instr_t _type)
{
    Instr* instr = new Instr(_type, m_instructions.size());
    m_instructions.emplace_back(instr);
    return instr;
}

bool Asm::Compiler::is_program_valid(const Node* _program)
{
    bool is_valid;

    // check if program an be run
    const VariableNodes& vars = _program->get<Scope>()->get_variables();
    bool found_a_var_uninit = false;
    auto it = vars.begin();
    while(!found_a_var_uninit && it != vars.end() )
    {
        if( !(*it)->isDeclared() )
        {
            LOG_ERROR("Compiler", "Unable to load program because %s is not declared.\n", (*it)->getName() );
            found_a_var_uninit = true;
        }
        ++it;
    }

    is_valid = !found_a_var_uninit;

    return is_valid;
}

Code* Asm::Compiler::get_output_assembly()
{
    return m_output;
}

void Asm::Compiler::append_to_assembly_code(const Node* _node)
{
    if ( _node )
    {
        if ( _node->get_class()->is<AbstractConditionalStruct>() )
        {
            auto cond = _node->as<AbstractConditionalStruct>();

            // for_loop init instruction
            if ( auto for_loop = _node->as<ForLoopNode>() )
            {
                auto init_instr = m_output->push_instr(Instr_t::call  );
                init_instr->m_left_h_arg = (i64_t)FctId::eval_member;
                init_instr->m_right_h_arg = (i64_t)for_loop->get_init_expr();
                init_instr->m_comment = "init-loop";
            }

            Instr* cond_instr = m_output->push_instr(Instr_t::call);
            cond_instr->m_left_h_arg = (i64_t)FctId::eval_member;
            cond_instr->m_right_h_arg = (i64_t)cond->get_condition();
            cond_instr->m_comment = "condition";

            Instr* store_instr = m_output->push_instr(Instr_t::mov);
            store_instr->m_left_h_arg = Register::rdx;
            store_instr->m_right_h_arg = Register::rax;
            store_instr->m_comment = "copy last result to a data register.";

            Instr* skip_true_branch = m_output->push_instr(Instr_t::jne);
            skip_true_branch->m_comment = "jump if register is false";

            Instr* skip_false_branch = nullptr;

            if ( auto true_branch = cond->get_condition_true_branch() )
            {
                append_to_assembly_code( true_branch->get_owner() );

                if ( auto for_loop = _node->as<ForLoopNode>() )
                {
                    // insert end-loop instruction.
                    auto end_loop_instr = m_output->push_instr(Instr_t::call);
                    end_loop_instr->m_left_h_arg = (i64_t)FctId::eval_member;
                    end_loop_instr->m_right_h_arg  = (i64_t)for_loop->get_iter_expr();

                    // insert jump to condition instructions.
                    auto loop_jump = m_output->push_instr(Instr_t::jmp);
                    loop_jump->m_left_h_arg = cond_instr->m_line - loop_jump->m_line;
                    loop_jump->m_comment = "jump back to loop begining";

                }
                else if (cond->get_condition_false_branch())
                {
                    skip_false_branch = m_output->push_instr(Instr_t::jmp);
                    skip_false_branch->m_comment = "jump false branch";
                }
            }

            skip_true_branch->m_left_h_arg = m_output->get_next_pushed_instr_index() - skip_true_branch->m_line;

            if ( auto false_branch = cond->get_condition_false_branch() )
            {
                append_to_assembly_code( false_branch->get_owner() );
                skip_false_branch->m_left_h_arg = m_output->get_next_pushed_instr_index() - skip_false_branch->m_line;
            }
        }
        else if (_node->has<Scope>() )
        {
            for( auto each : _node->get_children() )
            {
                append_to_assembly_code(each);
            }

            // unset scope's VariableNodes except if is main_scope
            if ( _node->get_parent() )
            {
                Instr *instr = m_output->push_instr(Instr_t::call);
                instr->m_left_h_arg = (i64_t) FctId::unset_variables;
                instr->m_right_h_arg = (i64_t) _node;
                instr->m_comment = "reset stack (unset VariableNodes)";
            }
        }
        else
        {
            Instr* instr = m_output->push_instr(Instr_t::call);
            instr->m_left_h_arg  = (i64_t)FctId::eval_member;
            instr->m_right_h_arg = (i64_t)_node->getProps()->get("value");
            instr->m_comment     = "Evaluate a member and store result.";
        }
    }
}

bool Asm::Compiler::create_assembly_code(const Node* _program)
{
    /*
     * Here we take the program's base scope node (a tree) and we flatten it to an
     * instruction list. We add some jump instruction in order to skip portions of code.
     * This works "a little bit" like a compiler, at least for the "tree to list" point of view.
     */
    delete m_output;
    m_output = new Code();

    try
    {
        append_to_assembly_code(_program);
        m_output->push_instr(Instr_t::ret);
    }
    catch ( const std::exception& e )
    {
        LOG_ERROR("Compiler", "Unable to create assembly code for program.");
        m_output = nullptr;
    }

    return m_output != nullptr;
}

