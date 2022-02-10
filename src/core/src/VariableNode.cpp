#include <nodable/VariableNode.h>
#include <nodable/Member.h>
#include <nodable/Log.h>

using namespace Nodable;

REFLECT_DEFINE_CLASS(VariableNode)

VariableNode::VariableNode(Reflect::Type _type)
    :
        Node("Variable"),
        m_type_token(nullptr),
        m_identifier_token(nullptr),
        m_assignment_operator_token(nullptr)
{
	m_value = m_props.add("value", Visibility::Always, _type, Way_InOut);
}

bool VariableNode::eval() const
{
    if ( !is_defined() )
    {
        Node::eval();
    }
    return true;
}

void VariableNode::set_name(const char* _name)
{
    m_name = _name;
    std::string str = m_value->getTypeAsString();
    str.append(" ");
    str.append( _name );
	setLabel(str);

	if (m_name.length() > 4)
    {
        setShortLabel(m_name.substr(0, 3).append("..").c_str());
    }
    else
    {
        setShortLabel(_name);
    }

    m_value->setSourceExpression(_name);
}
