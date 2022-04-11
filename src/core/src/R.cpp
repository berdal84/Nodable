#include <nodable/core/reflection/reflection>

#include <type_traits> // std::underlying_type
#include <stdexcept>   // std::runtime_error

using namespace Nodable;

type type::any  = type::get<any_t>();
type type::null = type::get<std::nullptr_t>();

bool type::is_ptr(type left)
{
    return left.m_is_pointer;
}

bool type::is_ref(type left)
{
    return left.m_is_reference;
}

bool type::is_implicitly_convertible(type _left, type _right )
{
    if(_left == type::get<any_t>() || _right == type::get<any_t>() ) // We allow cast to unknown type
    {
        return true;
    }
    else if (_left.get_underlying_type() == _right.get_underlying_type() )
    {
        return true;
    }
    else if (is_ptr(_left) && is_ptr(_right))
    {
        return true;
    }

    return     _left.get_underlying_type() == type::get<i16_t>()
           && _right.get_underlying_type() == type::get<double>();
}

type type::get_underlying_type() const
{
    return database::get(m_underlying_type);
}

std::string type::get_fullname() const
{
    std::string result;

    if (is_const())
    {
        result.append("const ");
    }

    result.append(m_name);

    if (is_ptr())
    {
        result.append("*");
    }
    else if (is_ref())
    {
        result.append("&");
    }

    return result;
}

bool type::is_ptr()const
{
    return m_is_pointer;
}

bool type::is_ref()const
{
    return m_is_reference;
}

type database::get(size_t _hash)
{
    return by_hash().find(_hash)->second;
}

std::map<size_t, type>& database::by_hash()
{
    static std::map<size_t, type> meta_type_register_by_typeid;
    return meta_type_register_by_typeid;
}

bool type::is_child_of(type _possible_parent_class, bool _selfCheck) const
{
    bool is_child;

    if (_selfCheck && m_hash_code == _possible_parent_class.m_hash_code )
    {
        is_child = true;
    }
    else if ( m_parents.empty())
    {
        is_child = false;
    }
    else
    {
        auto direct_parent_found = m_parents.find(_possible_parent_class.m_hash_code);

        // direct parent check
        if ( direct_parent_found != m_parents.end())
        {
            is_child = true;
        }
        else // indirect parent check
        {
            bool is_a_parent_is_child_of = false;
            for (auto each : m_parents)
            {
                type parent_type = database::get(each);
                if (parent_type.is_child_of(_possible_parent_class, true))
                {
                    is_a_parent_is_child_of = true;
                }
            }
            is_child = is_a_parent_is_child_of;
        }
    }
    return is_child;
};

void type::add_parent(type _parent)
{
    m_parents.insert(_parent.hash_code());
}

void type::add_child(type _child)
{
    m_children.insert( _child.hash_code() );
}

bool type::is_const() const
{
    return m_is_const;
}

bool database::has(type _type)
{
    return by_hash().find(_type.hash_code()) != by_hash().end();
}

bool database::has(size_t _hash_code)
{
    return by_hash().find(_hash_code) != by_hash().end();
}

void database::insert(type _type)
{
    NODABLE_ASSERT(!has(_type.hash_code()))
    by_hash().insert({_type.hash_code(), _type});
}

bool initializer::s_initialized = false;

initializer::initializer()
{
    if( s_initialized )
    {
        throw std::runtime_error("R has already been initialised !");
    }

    registration::push<double>("double");
    registration::push<std::string>("std::string");
    registration::push<bool>("bool");
    registration::push<void>("void");
    registration::push<i16_t>("i16_t");
    registration::push<type::any_t>("any");

    log_statistics();

    s_initialized = true;
}


void initializer::log_statistics()
{
    LOG_MESSAGE("R", "Logging reflected types ...\n");

    LOG_MESSAGE("R", "By typeid (%i):\n", database::by_hash().size() );
    for ( const auto& [type_hash, type] : database::by_hash() )
    {
        LOG_MESSAGE("R", " %llu => %s \n", type_hash, type.get_name() );
    }

    LOG_MESSAGE("R", "Logging done.\n");
}
