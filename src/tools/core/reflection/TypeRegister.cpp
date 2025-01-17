#include "TypeRegister.h"
#include "Type.h"
#include "tools/core/log.h"
#include "tools/core/assertions.h"

using namespace tools;

TypeDescriptor* TypeRegister::get(std::type_index index)
{
    auto found = by_index().find(index);
    VERIFY(found != by_index().end(), "reflection: type not found!");
    return found->second;
}

ClassDescriptor* TypeRegister::get_class(std::type_index index)
{
    TypeDescriptor* type = get(index);
    if ( type->is_class() )
        return static_cast<ClassDescriptor*>( type );
    return nullptr;
}

std::unordered_map<std::type_index, TypeDescriptor*>& TypeRegister::by_index()
{
    static std::unordered_map<std::type_index, TypeDescriptor*> meta_type_register_by_typeid;
    return meta_type_register_by_typeid;
}


bool TypeRegister::has(const TypeDescriptor* _type)
{
    return by_index().find(_type->id()) != by_index().end();
}

bool TypeRegister::has(std::type_index index)
{
    auto found = by_index().find(index);
    return found != by_index().end();
}

TypeDescriptor* TypeRegister::insert(TypeDescriptor* _type)
{
    by_index().insert({_type->id(), _type});
    return _type;
}

TypeDescriptor* TypeRegister::merge(TypeDescriptor* existing, const TypeDescriptor* other)
{
    TOOLS_DEBUG_LOG(
        TOOLS_VERBOSE,
        __FILE__,
        "Merge existing: \"%s\" (%s), with: \"%s\" (%s)\n",
        existing->m_name.c_str(), existing->m_compiler_name,
        other->m_name.c_str(), other->m_compiler_name
    );
    if( existing->m_name[0] == '\0' )
    {
        existing->m_name = other->m_name;
    }

    if ( existing->is_class() )
    {
        auto* existing_class = reinterpret_cast<ClassDescriptor*>(existing);
        auto* other_class    = reinterpret_cast<const ClassDescriptor*>(other);

        existing_class->m_children.insert(other_class->m_children.begin(), other_class->m_children.end() );
        existing_class->m_parents.insert(other_class->m_parents.begin(), other_class->m_parents.end() );
    }

    return existing;
}

void TypeRegister::log_statistics()
{
    TOOLS_LOG(TOOLS_MESSAGE, "reflection", "Logging reflected types ...\n");
    TOOLS_LOG(TOOLS_MESSAGE, "reflection", " %-16s %-25s %-60s\n", "-- type hash --", "-- user name --", "-- compiler name --" );

    for ( const auto& [type_hash, type] : by_index() )
    {
        TOOLS_LOG(TOOLS_MESSAGE, "reflection", " %-16llu %-25s %-60s\n", type_hash, type->m_name.c_str(), type->m_compiler_name );
    }

    TOOLS_LOG(TOOLS_MESSAGE, "reflection", "Logging done.\n");
}

TypeDescriptor* TypeRegister::insert_or_merge(TypeDescriptor* possibly_existing_type)
{
    if( has(possibly_existing_type->id()) )
    {
        TypeDescriptor* existing_type = get(possibly_existing_type->id());
        return merge(existing_type, possibly_existing_type);
    }
    return insert(possibly_existing_type);
}
