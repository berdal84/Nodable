#include <nodable/core/reflection/func_type.h>
#include <algorithm> // find_if
#include <nodable/core/constants.h>
#include <nodable/core/Operator.h>

using namespace Nodable;

func_type::func_type(std::string _id)
    : m_identifier(_id)
    , m_operator(nullptr)
    , m_return_type(type::null)
{
}

func_type::func_type(std::string _id, const Operator* _operator)
        : m_identifier(_id)
        , m_operator(_operator)
        , m_return_type(type::null)
{
    NODABLE_ASSERT(_operator)

    m_operator = _operator;
}

void func_type::push_arg(type _type)
{
   // create normalised name

   std::string name;

    if( m_operator)
    {
        switch ( m_args.size() + 1)
        {
            case 1: name = k_lh_value_member_name; break;
            case 2: name = k_rh_value_member_name; break;
            default: NODABLE_ASSERT(false)         break;
        }
    }
    else
    {
        name = k_func_arg_member_name_prefix + std::to_string(m_args.size());
    }

    m_args.emplace_back(_type, name);
}

bool func_type::is_exactly(const func_type* _other)const
{
    if ( this == _other )                        return true;
    if ( m_args.size() != _other->m_args.size()) return false;
    if ( m_identifier != _other->m_identifier )  return false;
    if ( m_args.empty() )                        return true;

    size_t i = 0;
    while( i < m_args.size() )
    {
        auto &arg_t       = m_args[i].m_type;
        auto &other_arg_t = _other->m_args[i].m_type;

        if ( arg_t != other_arg_t )
        {
            return false;
        }
        i++;
    }
    return true;
}

bool func_type::is_compatible(const func_type* _other)const
{
    if ( this == _other )                        return true;
    if ( m_args.size() != _other->m_args.size()) return false;
    if ( m_identifier != _other->m_identifier )  return false;
    if ( m_args.empty() )                        return true;

    size_t i = 0;
    while( i < m_args.size() )
    {
        auto &arg_t       = m_args[i].m_type;
        auto &other_arg_t = _other->m_args[i].m_type;

        if ( arg_t == other_arg_t )
        {
        }
        else if ( other_arg_t.is_ref() && type::is_implicitly_convertible(other_arg_t, arg_t))
        {
        }
        else
        {
            return false;
        }
        i++;
    }
    return true;

}

bool func_type::has_an_arg_of_type(type _type) const
{
    auto found = std::find_if( m_args.begin(), m_args.end(), [&_type](const FuncArg& each) { return each.m_type == _type; } );
    return found != m_args.end();
}

std::string func_type::get_label() const
{
    if( m_operator )
    {
        return m_operator->identifier;
    }
    return m_identifier;
}