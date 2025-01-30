#pragma once

#include "ASTNode.h"
#include "ASTVariable.h"

namespace ndbl
{
    class ASTNodeProperty;

    class ASTVariableRef : public ASTNode
    {
    public:

        ~ASTVariableRef()
        {
            clear_variable();
        }

        void init()
        {
            ASTNode::init(ASTNodeType_VARIABLE_REF, "");

            // Set name
            //set_name("Ref.");

            // Init identifier property
            value()->set_type(tools::type::any());
            value()->set_token({ASTToken_t::identifier});

            // Init Slots
            add_slot(value(), SlotFlag_FLOW_OUT, 1);
            add_slot(value(), SlotFlag_FLOW_IN , ASTNodeSlot::MAX_CAPACITY);
            add_slot(value(), SlotFlag_INPUT   , 1);
            add_slot(value(), SlotFlag_OUTPUT  , 1); // ref can be connected once
        }

        void set_variable(ASTVariable* variable)
        {
            VERIFY( m_variable == nullptr, "Can't call twice");

            m_variable = variable;
            value()->set_type( m_variable->get_type() );
            value()->token().word_replace( m_variable->get_identifier().c_str() );

            // bind signals
            m_variable->signal_name_change.connect<&ASTVariableRef::handle_name_change>(this);
            m_variable->signal_shutdown.connect<&ASTVariableRef::clear_variable>(this);
        }

        void clear_variable()
        {
            if ( m_variable == nullptr )
                return;

            // unbind signals
            assert(m_variable->signal_name_change.disconnect<&ASTVariableRef::handle_name_change>(this));
            assert(m_variable->signal_shutdown.disconnect<&ASTVariableRef::clear_variable>(this));

            m_variable = nullptr;
        }

        const ASTToken& get_identifier_token() const
        {
            return value()->token(); // when parsed, this token may be a bit different from m_variable's (trailing ignored characters)
        }

    private:
        void handle_name_change(const std::string& name)
        {
            value()->token().word_replace( name.c_str() );
        }

        ASTVariable* m_variable = nullptr;
    };
}