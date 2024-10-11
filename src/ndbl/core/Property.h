#pragma once

#include "ndbl/core/Token.h"
#include "tools/core/memory/memory.h"
#include "tools/core/reflection/variant.h"
#include "tools/core/types.h"// for constants and forward declarations

#include <string>
#include <vector>

namespace ndbl
{
    // forward declarations
    class Node;

    typedef int PropertyFlags;
    enum PropertyFlag_
    {
        PropertyFlag_NONE            = 0,
        PropertyFlag_IS_REF          = 1 << 0,
        PropertyFlag_IS_PRIVATE      = 1 << 1,
        PropertyFlag_IS_THIS         = 1 << 2, // Property pointing this Property's parent Node (stored as void* in variant).
        PropertyFlag_ALL             = ~PropertyFlag_NONE,
    };

    // Property wraps a Token including extra information such as: name, owner (Node), and some flags.
	class Property
    {
    public:
        void               init(const tools::TypeDesc*, PropertyFlags, Node*, const char* _name); // must be called once before use
        void               digest(Property *_property);
        bool               has_flags(PropertyFlags flags)const { return (m_flags & flags) == flags; };
        void               set_flags(PropertyFlags flags) { m_flags |= flags; }
        void               clear_flags(PropertyFlags flags = PropertyFlag_ALL) { m_flags &= ~flags; }
        //void               set_name(const char* _name) { m_name = _name; } names are indexed in PropertyBag, can't change
        const std::string& get_name()const { return m_name; }
        Node*              get_owner()const { return m_owner; }
        const tools::TypeDesc* get_type()const { return m_type; }
        bool               is_type(const tools::TypeDesc* other) const;
        void               set_token(const Token& _token) { m_token = _token; }
        Token&             get_token() { return m_token; }
        const Token&       get_token() const { return m_token; }

    private:
        Node*              m_owner = nullptr;
        PropertyFlags      m_flags = PropertyFlag_NONE;
        const tools::TypeDesc* m_type  = nullptr;
        std::string        m_name;
        Token              m_token;
    };
}