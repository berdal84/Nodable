#pragma once
#include <memory> // std::shared_ptr
#include "tools/core/reflection/reflection"
#include "ASTNode.h" // Base class

namespace ndbl
{
    class ASTLiteral: public ASTNode
    {
    public:
        DECLARE_REFLECT_override

        ASTToken token = {ASTToken_t::literal_any };

        ASTLiteral() {};
        ~ASTLiteral() override {};

        void init(const tools::TypeDescriptor* _type, const std::string& _name);

    private:
        const tools::TypeDescriptor* m_type = nullptr;
    };
}