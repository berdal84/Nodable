#include <Variable.h>
#include <Member.h>
#include <Log.h>

using namespace Nodable;

Variable::Variable():Node("Variable")
{
	add("value", Visibility::Always, Type::Any, Way_InOut);	
}

Variable::~Variable()
{

}


void Variable::setName(const char* _name)
{
	name = _name;
	setLabel(_name);
	get("value")->setSourceExpression(_name);
}

const char* Variable::getName()const
{
	return name.c_str();
}

bool Variable::isType(Type _type)const
{
	return value()->isType(_type);
}

std::string Variable::getTypeAsString()const
{
	return value()->getTypeAsString();
}
