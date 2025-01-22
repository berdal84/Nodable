#pragma once

#include "ndbl/core/ASTToken.h"
#include "tools/core/reflection/variant.h"
#include "tools/core/types.h"// for constants and forward declarations

#include <string>
#include <vector>

namespace ndbl
{
    // forward declarations
    class ASTNode;

    typedef int PropertyFlags;
    enum PropertyFlag_
    {
        PropertyFlag_NONE            = 0,
        PropertyFlag_IS_REF          = 1 << 0,
        PropertyFlag_IS_NODE_VALUE   = 1 << 1,
        PropertyFlag_ALL             = ~PropertyFlag_NONE,
    };

    // Property wraps a Token including extra information such as: name, owner (Node), and some flags.
	class ASTNodeProperty
    {
    public:
        ASTNodeProperty() = delete;
        explicit ASTNodeProperty(ASTNode* owner);
        void               init(const tools::TypeDescriptor*, PropertyFlags, const char* _name); // must be called once before use
        void               init_token();
        void               digest(ASTNodeProperty *_property);
        bool               has_flags(PropertyFlags flags)const { return (m_flags & flags) == flags; };
        void               set_flags(PropertyFlags flags) { m_flags |= flags; }
        void               clear_flags(PropertyFlags flags = PropertyFlag_ALL) { m_flags &= ~flags; }
        //void             set_name(const char* _name) { m_name = _name; } names are indexed in PropertyBag, can't change
        PropertyFlags      flags()const { return m_flags; }
        const std::string& name()const { return m_name; }
        ASTNode*           node()const { return m_node; }
        const tools::TypeDescriptor* get_type() const { return m_type; }
        bool               is_type(const tools::TypeDescriptor* other) const;
        void               set_type(const tools::TypeDescriptor *pDescriptor);
        void               set_token(const ASTToken& _token) { m_token = _token; }
        ASTToken&          token() { return m_token; }
        const ASTToken&    token() const { return m_token; }

    private:
        ASTToken           m_token;
        ASTNode*           m_node; // owner
        PropertyFlags      m_flags = PropertyFlag_NONE;
        std::string        m_name;
        const tools::TypeDescriptor* m_type = nullptr;
    };
}