#pragma once

#include "Action.h"

namespace ndbl
{
    class ASTNodeViewContextualMenu
    {
    public:
        Action_CreateNode*       draw_search_input(ASTNodeSlotView* _dragged_slot, size_t _result_max_count ); // Return the triggered action, user has to deal with the Action.
        void                     flag_to_be_reset();
        void                     add_action(Action_CreateNode*);

    private:
        bool      must_be_reset_flag   = true;
        char      search_input[255]    = "\0";     // The search input entered by the user.
        std::vector<Action_CreateNode*> items;                           // All the available items
        std::vector<Action_CreateNode*> items_with_compatible_signature; // Only the items having a compatible signature (with the slot dragged)
        std::vector<Action_CreateNode*> items_matching_search;           // Only the items having a compatible signature AND matching the search_input.
        void                     update_cache_based_on_signature(ASTNodeSlotView* _dragged_slot);
        void                     update_cache_based_on_user_input(ASTNodeSlotView* _dragged_slot, size_t _limit );
    };
}