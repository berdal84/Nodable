#pragma once
#include "tools/core/Component.h"
#include "ASTNode.h"
#include "ASTNodeSlot.h"
#include "ASTScope.h"

namespace ndbl
{
    class ASTVariable;

    namespace ASTUtils
    {
        template<typename C>
        C* adjacent_component_at(const ASTNode*, SlotFlags, u8_t pos);

        template<typename C>
        static std::vector<C*> adjacent_components(const ASTNode*, SlotFlags);

        std::vector<ASTNode*> get_adjacent_nodes(const ASTNode*, SlotFlags);
        ASTNode*              adjacent_node_at(const ASTNode*, SlotFlags, u8_t pos);
        bool               is_instruction(const ASTNode*);
        bool               can_be_instruction(const ASTNode*);
        bool               is_unary_operator(const ASTNode*);
        bool               is_binary_operator(const ASTNode*);
        bool               is_conditional(const ASTNode*);
        bool               is_connected_to_codeflow(const ASTNode *node);
        bool               is_output_node_in_expression(const ASTNode* input_node, const ASTNode* output_node);
    }

    template<typename ComponentT>
    ComponentT* ASTUtils::adjacent_component_at(const ASTNode* _node, SlotFlags _flags, u8_t _pos)
    {
        if( ASTNode* adjacent_node = adjacent_node_at(_node, _flags, _pos) )
        {
            return adjacent_node->component<ComponentT>();
        }
        return {};
    }

    template<typename ComponentT>
    static std::vector<ComponentT*> ASTUtils::adjacent_components(const ASTNode* _node, SlotFlags _flags)
    {
        std::vector<ComponentT*> result;
        for(auto _adjacent_node : get_adjacent_nodes( _node, _flags ) )
            if( ComponentT* component = _adjacent_node->component<ComponentT>() )
                result.push_back( component );

        return result;
    }
}
