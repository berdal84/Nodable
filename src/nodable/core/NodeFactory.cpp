#include "NodeFactory.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "fw/core/reflection/reflection"

#include "InstructionNode.h"
#include "VariableNode.h"
#include "LiteralNode.h"
#include "InvokableComponent.h"
#include "Scope.h"
#include "ConditionalStructNode.h"
#include "ForLoopNode.h"
#include "WhileLoopNode.h"
#include "NodeUtils.h"

using namespace ndbl;
using fw::Pool;


NodeFactory::NodeFactory()
: m_post_process([](PoolID<Node> _node){})
, m_post_process_is_overrided(false)
{}

PoolID<InstructionNode> NodeFactory::create_instr() const
{
    auto node = Pool::get_pool()->create<InstructionNode>();
    node->init();
    node->set_name("Instr.");
    m_post_process(node);
    return node;
}

PoolID<VariableNode> NodeFactory::create_variable(const fw::type *_type, const std::string& _name, PoolID<Scope> _scope) const
{
    // create
    auto node = Pool::get_pool()->create<VariableNode>(_type, _name.c_str());
    node->init();
    if( _scope )
    {
        _scope->add_variable( node );
    }
    else
    {
        LOG_WARNING("HeadlessNodeFactory", "Variable %s has been created without defining its scope.\n", _name.c_str())
    }

    m_post_process(node);

    return node;
}

PoolID<Node> NodeFactory::create_abstract_func(const fw::func_type* _signature, bool _is_operator) const
{
    PoolID<Node> node = _create_abstract_func(_signature, _is_operator);
    add_invokable_component(node, _signature, nullptr, _is_operator );
    m_post_process(node);
    return node;
}

PoolID<Node> NodeFactory::_create_abstract_func(const fw::func_type* _func_type, bool _is_operator) const
{
    PoolID<Node> node = Pool::get_pool()->create<Node>();
    node->init();

    if( _is_operator )
    {
        node->set_name(_func_type->get_identifier().c_str());
    }
    else
    {
        std::string id = _func_type->get_identifier();
        std::string label       = id + "()";
        std::string short_label = id.substr(0, 2) + "..()"; // ------- improve, not great.
        node->set_name(label.c_str());
    }

    // Create a result/value
    auto return_prop_id = node->props.add(_func_type->get_return_type(), VALUE_PROPERTY );
    node->add_slot( return_prop_id, SlotFlag_OUTPUT, SLOT_MAX_CAPACITY );

    // Create arguments
    auto args = _func_type->get_args();

    if( _is_operator ) // rename arguments
    {
        size_t count = _func_type->get_arg_count();

        FW_EXPECT( count != 0 , "An operator cannot have zero argument" );
        FW_EXPECT( count < 3  , "An operator cannot have more than 2 arguments" );

        switch ( count )
        {
            case 1:
            {
                auto left_prop_id = node->props.add(args[0].m_type, LEFT_VALUE_PROPERTY );
                node->add_slot( left_prop_id, SlotFlag_INPUT, 1 );
                break;
            }

            case 2:
            {
                auto left_prop_id = node->props.add( args[0].m_type, LEFT_VALUE_PROPERTY );
                auto right_prop_id = node->props.add( args[1].m_type, RIGHT_VALUE_PROPERTY );
                node->add_slot( left_prop_id, SlotFlag_INPUT, 1 );
                node->add_slot( right_prop_id, SlotFlag_INPUT, 1 );
                break;
            }

            default: /* no warning */ ;
        }
    }
    else
    {
        for (auto& arg : args)
        {
            auto each_prop_id = node->props.add(arg.m_type, arg.m_name.c_str() );
            node->add_slot( each_prop_id, SlotFlag_INPUT, 1 );
        }
    }

    return node;
}

PoolID<Node> NodeFactory::create_func(const fw::iinvokable* _function, bool _is_operator) const
{
    // Create an abstract function node
    const fw::func_type* type = _function->get_type();
    PoolID<Node> node = _create_abstract_func(type, _is_operator);
    add_invokable_component(node, type, _function, _is_operator);
    m_post_process(node);
    return node;
}

void NodeFactory::add_invokable_component(PoolID<Node> _node, const fw::func_type* _func_type, const fw::iinvokable *_invokable, bool _is_operator) const
{
    // Create an InvokableComponent with the function.
    PoolID<InvokableComponent> component = Pool::get_pool()->create<InvokableComponent>(_func_type, _is_operator, _invokable);
    _node->add_component(component);

    // Bind result property
    Slot* result_slot = _node->find_slot( VALUE_PROPERTY, SlotFlag_OUTPUT );
    component->bind_result( *result_slot );

    // Link arguments
    auto args = _func_type->get_args();
    for (u8_t index = 0; index < (u8_t)args.size(); index++)
    {
        Slot& arg_slot = _node->find_nth_slot(index, SlotFlag_INPUT );
        if ( args[index].m_by_reference )
        {
            arg_slot.get_property()->flag_as_reference();  // to handle by reference function args
        }
        component->bind_arg(index, arg_slot );
    }
}

PoolID<Node> NodeFactory::create_scope() const
{
    PoolID<Node> node = Pool::get_pool()->create<Node>();
    node->init();
    node->set_name("{} Scope");

    node->set_slot_capacity( SlotFlag_PREV, SLOT_MAX_CAPACITY );
    node->set_slot_capacity( SlotFlag_NEXT, 1 );

    PoolID<Scope> scope_id = Pool::get_pool()->create<Scope>();
    node->add_component(scope_id);

    m_post_process(node);

    return node;
}

PoolID<ConditionalStructNode> NodeFactory::create_cond_struct() const
{
    PoolID<ConditionalStructNode> node = Pool::get_pool()->create<ConditionalStructNode>();
    node->init();
    node->set_name("If");
    node->add_component(Pool::get_pool()->create<Scope>());

    m_post_process(node);

    return node;
}

PoolID<ForLoopNode> NodeFactory::create_for_loop() const
{
    PoolID<ForLoopNode> node = Pool::get_pool()->create<ForLoopNode>();
    node->init();
    node->set_name("For");
    node->add_component(Pool::get_pool()->create<Scope>());

    m_post_process(node);

    return node;
}

PoolID<WhileLoopNode> NodeFactory::create_while_loop() const
{
    PoolID<WhileLoopNode> node = Pool::get_pool()->create<WhileLoopNode>();
    node->init();
    node->set_name("While");
    node->add_component(Pool::get_pool()->create<Scope>());

    m_post_process(node);

    return node;
}

PoolID<Node> NodeFactory::create_program() const
{
    PoolID<Node> node = create_scope(); // A program is a main scope.
    node->set_name(ICON_FA_FILE_CODE " Program");
    node->set_slot_capacity(SlotFlag_PREV, 0);
    node->set_slot_capacity(SlotFlag_PARENT, 0);

    m_post_process(node);
    return node;
}

PoolID<Node> NodeFactory::create_node() const
{
    PoolID<Node> node = Pool::get_pool()->create<Node>();
    node->init();
    m_post_process(node);
    return node;
}

PoolID<LiteralNode> NodeFactory::create_literal(const fw::type *_type) const
{
    PoolID<LiteralNode> node = Pool::get_pool()->create<LiteralNode>(_type);
    node->init();
    node->set_name("Literal");
    m_post_process(node);
    return node;
}

void NodeFactory::destroy_node(PoolID<Node> node) const
{
    Pool* pool = Pool::get_pool();
    pool->destroy_all( node->get_components() );
    pool->destroy( node );
}

void NodeFactory::override_post_process_fct( NodeFactory::PostProcessFct f)
{
    FW_EXPECT( m_post_process_is_overrided == false, "Cannot override post process function more than once." );
    m_post_process_is_overrided = true;
    m_post_process              = f;
}
