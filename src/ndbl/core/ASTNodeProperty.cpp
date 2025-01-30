#include "ASTNodeProperty.h"
#include "ASTVariable.h"
#include "ndbl/core/language/Nodlang.h"

using namespace ndbl;
using namespace tools;

ASTNodeProperty::ASTNodeProperty(ASTNode* owner)
: m_node(owner)
, m_token()
{}

void ASTNodeProperty::init(const TypeDescriptor* _type, PropertyFlags _flags, const char* _name)
{
    VERIFY(m_type == nullptr, "must be initialized once");
    VERIFY(_type != nullptr, "type can't be nullptr");
    m_type  = _type;
    m_flags = _flags;
    m_name  = _name;

    // Ensure token matches with Property type
    init_token();
}

void ASTNodeProperty::init_token()
{
    const Nodlang* language = get_language();

    // Convert m_type to a Token_t
    ASTToken_t token_type = language->to_literal_token(m_type );
    VERIFY(token_type != ASTToken_t::none, "This token is not handled");

    m_token = { token_type };
}

void ASTNodeProperty::digest(ASTNodeProperty* _property)
{
    m_token = std::move( _property->m_token );
}

bool ASTNodeProperty::is_type(const TypeDescriptor* other) const
{
    return m_type->equals( other );
}

void ASTNodeProperty::set_type(const tools::TypeDescriptor* type)
{
    bool change = type != m_type;
    m_type = type;
    if (change)
        init_token();
}
