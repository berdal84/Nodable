#include "ASTNodeViewContextualMenu.h"

using namespace ndbl;
using namespace tools;

void ASTNodeViewContextualMenu::update_cache_based_on_signature(ASTNodeSlotView* dragged_slot)
{
    items_with_compatible_signature.clear();

    // 1) When NO slot is dragged
    //---------------------------

    if ( !dragged_slot )
    {
        // When no slot is dragged, user can create any node
        items_with_compatible_signature = items;
        return;
    }

    // 2) When a slot is dragged
    //--------------------------

    for (auto& action: items )
    {
        const TypeDescriptor* dragged_property_type = dragged_slot->property_type();

        switch ( action->event_data.node_type )
        {
            case CreateNodeType_BLOCK_CONDITION:
            case CreateNodeType_BLOCK_FOR_LOOP:
            case CreateNodeType_BLOCK_WHILE_LOOP:
            case CreateNodeType_BLOCK_SCOPE:
            case CreateNodeType_ROOT:
                // Blocks are only for code flow slots
                if ( !dragged_slot->allows(SlotFlag_TYPE_FLOW) )
                    continue;
                break;

            default:

                if ( dragged_slot->allows(SlotFlag_TYPE_FLOW))
                {
                    // we can connect anything to a code flow slot
                }
                else if ( dragged_slot->allows(SlotFlag_INPUT) && dragged_slot->property_type()->is<ASTNode*>() )
                {
                    // we can connect anything to a Node ref input
                }
                else if ( action->event_data.node_signature )
                {
                    // discard incompatible signatures

                    if ( dragged_slot->allows(SlotFlag_ORDER_1ST ) &&
                        !action->event_data.node_signature->has_arg_with_type(dragged_property_type)
                            )
                        continue;

                    if ( !action->event_data.node_signature->return_type()->equals(dragged_property_type) )
                        continue;

                }
        }
        items_with_compatible_signature.push_back( action );
    }
}

template<typename charT>
struct CaseInsensitiveEqual
{
    const std::locale& locale;
    bool operator()(charT ch1, charT ch2)
    {
        return std::toupper(ch1, locale) == std::toupper(ch2, locale);
    }
};

template<typename T>
bool CaseInsensitiveFind(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
    return std::search(str1.begin(), str1.end(),
                       str2.begin(), str2.end(),
                       CaseInsensitiveEqual<typename T::value_type>{loc}) != str1.end();
}

void ASTNodeViewContextualMenu::update_cache_based_on_user_input(ASTNodeSlotView* _dragged_slot, size_t _limit )
{
    std::string search{search_input}; // FindCaseInsensitive takes a std::string
    items_matching_search.clear();
    for ( auto& menu_item : items_with_compatible_signature )
    {
        if( !CaseInsensitiveFind(menu_item->label, search) )
            continue;

        items_matching_search.push_back(menu_item);
        if ( items_matching_search.size() == _limit )
            break;
    }
}

void ASTNodeViewContextualMenu::flag_to_be_reset()
{
    must_be_reset_flag   = true;
}

void ASTNodeViewContextualMenu::add_action(Action_CreateNode* action)
{
    items.push_back(action);
}


Action_CreateNode* ASTNodeViewContextualMenu::draw_search_input(ASTNodeSlotView* dragged_slot, size_t _result_max_count )
{
    if ( must_be_reset_flag )
    {
        search_input[0]      = '\0';
        items_matching_search.clear();
        items_with_compatible_signature.clear();

        ImGui::SetKeyboardFocusHere();

        //
        update_cache_based_on_signature(dragged_slot);

        // Initial search
        update_cache_based_on_user_input(dragged_slot, 100 );

        // Ensure we reset once
        must_be_reset_flag = false;
    }

    // Draw search input and update_cache_based_on_user_input on input change
    ImGui::BeginGroup();
    ImGui::Text("Create Node:");
    ImGui::SameLine();
    if ( ImGui::InputText("###Search", search_input, 255, ImGuiInputTextFlags_EscapeClearsAll ))
    {
        update_cache_based_on_user_input(dragged_slot, 100 );
    }
    ImGui::EndGroup();

    if ( !items_matching_search.empty() )
    {
        // When a single item is filtered, pressing enter will press the item's button.
        if ( items_matching_search.size() == 1)
        {
            auto action = items_matching_search.front();
            if ( ImGui::SmallButton( action->label.c_str()) || ImGui::IsKeyDown( ImGuiKey_Enter ) )
            {
                return action;
            }
        }
        else
        {
            size_t more = items_matching_search.size() > _result_max_count ? items_matching_search.size() : 0;
            if ( more )
            {
                ImGui::Text("Found %zu result(s)", items_matching_search.size() );
            }
            // Otherwise, user has to move with arrow keys and press enter to trigger the highlighted button.
            auto it = items_matching_search.begin();
            while( it != items_matching_search.end() && std::distance(items_matching_search.begin(), it) != _result_max_count)
            {
                auto* action = *it;

                // User can click on the button...
                ImGui::Button( action->label.c_str());
                if( ImGui::IsItemClicked(0) )
                    return action;

                // ...or press enter if this item is the first
                if ( ImGui::IsKeyDown( ImGuiKey_Enter ) && ImGui::IsItemFocused() )
                    return action;

                it++;
            }
            if ( more )
            {
                ImGui::Text(".. %zu more ..", more );
            }
        }
    }
    else
    {
        ImGui::Text("No matches...");
    }

    return nullptr;
}