#pragma once
#include <memory>     // std::shared_ptr
#include <functional> // std::function

#include "tools/core/reflection/reflection"

#include "ASTForLoop.h"
#include "ASTIf.h"
#include "ASTWhileLoop.h"
#include "ASTFunctionCall.h"
#include "ASTVariableRef.h"

namespace ndbl
{
    // forward declarations
    class Nodlang;
    class ASTLiteral;
    class ASTForLoop;
    class ASTWhileLoop;
    class ASTIf;
    class ASTVariableRef;

    /**
     * @brief The NodeFactory instantiate Nodes. Class take a function to update_world_matrix after creation.
     *
     * By default post processing function does nothing. It can be used to add a NodeView, log messages, etc.
     */
    class ASTNodeFactory
    {
        typedef std::function<void(ASTNode*)> PostProcessFct;
    public:
        ASTNodeFactory();
        ~ASTNodeFactory(){}

        ASTNode*              create_root_scope()const;
        ASTNode*              create_scope()const;
        ASTVariable*          create_variable(const tools::TypeDescriptor *_type, const std::string &_name)const;
        ASTVariableRef*       create_variable_ref() const;
        ASTLiteral*           create_literal(const tools::TypeDescriptor *_type)const;
        ASTFunctionCall*      create_function(const tools::FunctionDescriptor&, ASTNodeType node_type = ASTNodeType_FUNCTION)const;
        ASTIf*                create_cond_struct()const;
        ASTForLoop*           create_for_loop()const;
        ASTWhileLoop*         create_while_loop()const;
        ASTNode*              create_node()const;
        ASTNode*              create_empty_instruction()const;
        void                  override_post_process_fct(PostProcessFct f);

    private:
        bool                   m_post_process_is_overrided;
        std::function<void(ASTNode*)>  m_post_process; // invoked after each node creation, just before to return.
    };

    [[nodiscard]]
    ASTNodeFactory* get_node_factory();
    ASTNodeFactory* init_node_factory();
    void         shutdown_node_factory(ASTNodeFactory *pFactory); // Undo init_node_factory()
}