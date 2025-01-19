#include "ASTFunctionCall.h"

#include "tools/core/log.h"
#include "tools/core/reflection/Type.h"

using namespace ndbl;
using namespace tools;

REFLECT_STATIC_INITIALIZER
(
    DEFINE_REFLECT(ASTFunctionCall).extends<ASTNode>();
)

void ASTFunctionCall::init(ASTNodeType _type, const tools::FunctionDescriptor& _func_type )
{
    ASTNode::init(_type, _func_type.get_identifier());

    m_func_type = _func_type;
    m_identifier_token = {
            ASTToken_t::identifier,
            _func_type.get_identifier()
    };
    m_argument_slot.resize(_func_type.arg_count());
    m_argument_props.resize(_func_type.arg_count());

    switch ( _type )
    {
        case ASTNodeType_OPERATOR:
            set_name(_func_type.get_identifier());
            break;
        case ASTNodeType_FUNCTION:
        {
            const std::string& id   = _func_type.get_identifier();
            std::string label       = id; // We add dynamically the brackets (see NodeView)
            std::string short_label = id.substr(0, 2) + "..";
            set_name(label.c_str());
            break;
        }
        default:
            VERIFY(false, "Type not allowed");
    }

    // Create a result/value
    value()->set_type(_func_type.return_type() );

    add_slot(value(), SlotFlag_OUTPUT   , ASTNodeSlot::MAX_CAPACITY );
    add_slot(value(), SlotFlag_FLOW_OUT , 1);
    add_slot(value(), SlotFlag_FLOW_IN  , ASTNodeSlot::MAX_CAPACITY );

    // Create arguments
    if (_type == ASTNodeType_OPERATOR )
    {
        VERIFY(_func_type.arg_count() >= 1, "An operator must have one argument minimum");
        VERIFY(_func_type.arg_count() <= 2, "An operator cannot have more than 2 arguments");
    }

    for (size_t i = 0; i < _func_type.arg_count(); i++ )
    {
        const FuncArg& arg  = _func_type.arg_at(i);

        const char* name;
        // TODO: this could be done in the NodeView instead...
        if (_type == ASTNodeType_OPERATOR )
        {
            if ( i == 0 )
                name = LEFT_VALUE_PROPERTY ;
            else if ( i == 1 )
                name = RIGHT_VALUE_PROPERTY;
        }
        else
        {
            name = arg.name.c_str();
        }

        ASTNodeProperty* property  = add_prop(arg.type, name );

        if ( arg.pass_by_ref )
            property->set_flags(PropertyFlag_IS_REF);

        m_argument_slot[i]  = add_slot(property, SlotFlag_INPUT, 1);
        m_argument_props[i] = property;
    }
}