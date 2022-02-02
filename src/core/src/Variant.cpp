#include <nodable/Variant.h>
#include <nodable/Log.h> // for LOG_DEBUG(...)
#include <cassert>
#include <nodable/Nodable.h>
#include <nodable/String.h>

using namespace Nodable;

Variant::Variant(): m_isDefined(false)
{
}

Variant::~Variant(){};

Type Variant::getType()const
{
	return Variant::s_nodableTypeByIndex.at(data.index());
}

bool  Variant::isType(Type _type)const
{
	return getType() == _type;
}

void Variant::set(double _var)
{
	switch( getType() )
	{
		case Type_String:
		{
			data.emplace<std::string>( std::to_string(_var) );
			break;
		}

        default:
		{
			data.emplace<double>( _var );
			break;
		}
	}
	m_isDefined = true;
}

void Variant::set(const std::string& _var)
{
   this->set(_var.c_str());
}

void Variant::set(const char* _var)
{
    switch (getType())
    {
        case Type_String:
        {
            data = _var;
        }

        default:
        {
            data = std::string(_var);
        }
    }
    m_isDefined = true;
}

void Variant::set(bool _var)
{
	switch(getType())
	{
		case Type_String:
		{
			data.emplace<std::string>( _var ? "true" : "false" );
			break;
		}

		case Type_Double:
		{
			data.emplace<double>( _var ? double(1) : double(0) );
			break;
		}

		default:
		{
			data.emplace<bool>( _var );
			break;
		}
	}
    m_isDefined = true;
}

bool Variant::isDefined()const
{
	return m_isDefined;
}

void Variant::undefine()
{
	m_isDefined = false;
}

void Variant::set(const Variant* _other)
{
	data = _other->data;
    m_isDefined = _other->m_isDefined;
    setType(_other->getType());
}

std::string Variant::getTypeAsString()const
{
	switch(getType())
	{
		case Type_String:	{return "string";}
		case Type_Double:	{return "double";}
		case Type_Boolean: 	{return "boolean";}
		default:				{return "unknown";}
	}
}

void Variant::setType(Type _type)
{
	if (getType() != _type)
	{
		undefine();

		// Set a default value (this will change the type too)
		switch (_type)
		{
		case Type_String:
			data.emplace<std::string>();
			break;
		case Type_Double:
			data.emplace<double>();
			break;
		case Type_Boolean:
			data.emplace<bool>();
			break;
		default:
            data.emplace<Any>();
			break;
		}
	}

}

template<>
[[nodiscard]] i64_t Variant::convert_to<i64_t>()const
{
    switch (getType())
    {
        case Type_String:  return double( mpark::get<std::string>(data).size());
        case Type_Double:  return mpark::get<double>(data);
        case Type_Boolean: return mpark::get<bool>(data);
        default:           return double(0);
    }
}

template<>
[[nodiscard]] double Variant::convert_to<double>()const
{
    switch (getType())
    {
        case Type_String:  return double( mpark::get<std::string>(data).size());
        case Type_Double:  return mpark::get<double>(data);
        case Type_Boolean: return mpark::get<bool>(data);
        default:           return double(0);
    }
}

template<>
[[nodiscard]] int Variant::convert_to<int>()const
{
	return (int)this->convert_to<double>();
}

template<>
[[nodiscard]] bool Variant::convert_to<bool>()const
{
    switch (getType())
    {
        case Type_String:  return !mpark::get<std::string>(data).empty();
        case Type_Double:  return mpark::get<double>(data) != 0.0F;
        case Type_Boolean: return mpark::get<bool>(data);
        default:           return false;
    }
}

template<>
[[nodiscard]] std::string Variant::convert_to<std::string>()const
{
    switch (getType())
    {
        case Type_String:
        {
            return mpark::get<std::string>(data);
        }

        case Type_Double:
        {
            return String::from(mpark::get<double>(data));
        }

        case Type_Boolean:
        {
            return  mpark::get<bool>(data) ? "true" : "false";
        }

        default:
        {
            return "";
        }
    }
}

Variant::operator int()const          { return (int)(double)*this; }
Variant::operator double()const       { return convert_to<double>(); }
Variant::operator bool()const         { return convert_to<bool>(); }
Variant::operator std::string ()const { return convert_to<std::string>(); }

void Variant::define()
{
    /* declare variant as "defined" without affecting a value, existing data will be used */
    m_isDefined = true;
}
