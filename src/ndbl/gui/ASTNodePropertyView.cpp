#include "ASTNodePropertyView.h"

#include "tools/core/Component.h"
#include "ndbl/core/language/Nodlang.h"
#include "ndbl/core/ASTNode.h"
#include "ASTNodeView.h"
#include "Config.h"

using namespace ndbl;
using namespace tools;

constexpr Vec2  PROPERTY_TOGGLE_BTN_SIZE = Vec2(12.0, 22.0f);
constexpr float PROPERTY_INPUT_PADDING   = 5.0f;
constexpr float PROPERTY_INPUT_SIZE_MIN  = 12.0f;

ASTNodePropertyView::ASTNodePropertyView(ASTNodeProperty* _property )
: _property(_property)
, show(false)
, touched(false)
, _state()
, _shape(Vec2{10.f, 10.f})
{
}

void ASTNodePropertyView::reset()
{
    touched    = false;
    show = false;
}

ASTNodeProperty* ASTNodePropertyView::get_property() const
{
    return _property;
}

ASTNode* ASTNodePropertyView::get_node() const
{
    return _property->node();
}

bool ASTNodePropertyView::has_input_connected() const
{
    return get_node()->has_input_connected(_property );
}

ASTNodeSlot* ASTNodePropertyView::get_connected_slot() const
{
    const ASTNodeSlot* input_slot = get_node()->find_slot_by_property(_property, SlotFlag_INPUT );
    if( !input_slot )
        return nullptr;

    return input_slot->first_adjacent();
}

ASTVariable* ASTNodePropertyView::get_connected_variable() const
{
    ASTNodeSlot* adjacent_slot = get_connected_slot();
    if( !adjacent_slot )
        return nullptr;

    return cast<ASTVariable>(adjacent_slot->node);
}

bool ASTNodePropertyView::draw(ViewDetail _detail)
{
    _shape.draw_debug_info();

    if ( !_state.visible() )
        return false;

    bool               changed            = false;
    ASTNodeProperty*   property           = get_property();
    ASTNode*           node               = get_node();
    ASTNodeType        node_type          = node->type();

    /*
     * Handle input visibility
     */
    if ( _detail == ViewDetail::MINIMALIST )
    {
        this->show = false;
        this->show |= node_type == ASTNodeType_VARIABLE;
        this->show |= node_type == ASTNodeType_VARIABLE_REF;
    }
    else
    {
        // When untouched, it depends...

        this->show |= node_type == ASTNodeType_LITERAL;
        this->show |= node_type == ASTNodeType_VARIABLE;
        this->show |= node_type == ASTNodeType_VARIABLE_REF;

        // Always show when connected to a variable
        if ( const ASTNodeSlot* connected_slot = get_connected_slot() )
            switch ( connected_slot->node->type() )
            {
                case ASTNodeType_VARIABLE:
                case ASTNodeType_VARIABLE_REF:
                    this->show |= true;
            }

        // Always show properties that have an input slot free
        if (auto* slot = node->find_slot_by_property(property, SlotFlag_INPUT))
            this->show |= !slot->is_full();

        this->show |= this->touched;
    }

    // input
    if ( this->show )
    {
        const bool compact_mode = true;
        changed = ASTNodePropertyView::draw_input(this, compact_mode, nullptr);

        if ( ImGui::IsItemFocused() )
        {
            this->show    = false;
            this->touched = false;
        }
    }
    else
    {
        ImGui::Button("", { 8.f, 24.f} );

        if ( ImGui::IsItemClicked(0) )
        {
            get_config()->ui_node_detail = ViewDetail::NORMAL;
            this->touched = true;
            this->show    = true;
        }

    }

    if ( ImGuiEx::BeginTooltip() )
    {
        ImGui::Text("%s %s\n", property->get_type()->name(), property->name().c_str());

        std::string  source_code;
        if( property == node->value() || node->find_slot_by_property( property, SlotFlag_OUTPUT ))
            get_language()->serialize_node(source_code, node, SerializeFlag_RECURSE);
        else
            get_language()->serialize_property(source_code, property );

        ImGui::Text("source: \"%s\"", source_code.c_str());

        ImGuiEx::EndTooltip();
    }

    // Memorize new size and position fo this property
    const Vec2 new_size = ImGui::GetItemRectSize();
    const Vec2 new_pos  = ImGui::GetItemRectMin() + ImGui::GetItemRectSize() * 0.5f;
    _shape.set_position(new_pos, WORLD_SPACE); // GetItemRectMin is in SCREEN_SPACE
    _shape.set_size({new_size.x, node->component<ASTNodeView>()->shape()->size().y}); // We always want the box to fit with the node, it's easier to align things on it

#if DEBUG_DRAW
    ImGuiEx::DebugCircle( rect.center(), 2.5f, ImColor(0,0,0));
#endif
    return changed;
}

float ASTNodePropertyView::calc_input_width(const char *buf)
{
    return PROPERTY_INPUT_PADDING + std::max(ImGui::CalcTextSize(buf).x, PROPERTY_INPUT_SIZE_MIN);
}

bool ASTNodePropertyView::draw_input(ASTNodePropertyView* _view, bool _compact_mode, const char* _override_label)
{
    ASTNodeProperty*           property       = _view->get_property();
    ASTToken&              property_token = property->token();
    const ASTNodeSlot*         connected_slot = _view->get_connected_slot();
    ImGuiInputTextFlags flags          = ImGuiInputTextFlags_ReadOnly * (connected_slot != nullptr);
    std::string         label;

    // Create a label (everything after ## will not be displayed)
    if ( _override_label != nullptr )
        label.append(_override_label);
    else
        label.append("##" + property->name());

    //
    // Strategy:
    // 1) if property is connected to an identifier, we just display the value in an InputText (as read-only)
    // 2) if property is an identifier, or a literal we allow edition via an InputText, InputDouble/Int or Checkbox

    // 1
    if (property->node()->type() != ASTNodeType_VARIABLE)
        if ( connected_slot != nullptr )
            switch (connected_slot->node->type())
            {
                case ASTNodeType_VARIABLE:
                case ASTNodeType_VARIABLE_REF:
                {
                    char buf[256];
                    const ASTToken &connected_property_token = connected_slot->property->token();
                    snprintf(buf, std::min(connected_property_token.word_len() + 1, sizeof(buf)), "%s",
                             connected_property_token.word());
                    float w = calc_input_width(buf);
                    ImGui::PushItemWidth(w);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg,
                                          connected_slot->node->component<ASTNodeView>()->get_color(Color_FILL));
                    if (ImGui::InputText(label.c_str(), buf, sizeof(buf), flags))
                    {
                        // is ReadOnly
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopItemWidth();
                    return false;
                }
            }

    // 2)

    // Common
    bool changed = false;

    if ( _compact_mode )
    {
        std::string token_word = property_token.word_to_string();
        float w = calc_input_width(token_word.c_str());
        ImGui::PushItemWidth(w);
    }

    // Per type
    switch ( property_token.m_type )
    {
        case ASTToken_t::identifier:
        {
            char buf[256];
            snprintf(buf, std::min(property_token.word_len() + 1, sizeof(buf)), "%s", property_token.word());
            flags = 0; // ReadOnly always OFF. ImGuiInputTextFlags_ReadOnly * (connected_slot != nullptr);
            if (ImGui::InputText(label.c_str(), buf, sizeof(buf), flags))
            {
                property_token.word_replace(buf);
                changed = true;
            }
            break;
        }

        case ASTToken_t::literal_double:
        {

            double value = get_language()->parse_double_or(property_token.word_to_string(), 0);

            if (ImGui::InputDouble(label.c_str(), &value, 0.0, 0.0, "%.6f", flags))
            {
                std::string str;
                get_language()->serialize_double(str, value);
                property_token.word_replace(str.c_str());
                changed = true;
            }
            break;
        }

        case ASTToken_t::literal_int:
        {
            i32_t value = get_language()->parse_int_or( property_token.word_to_string(), 0);

            if (ImGui::InputInt(label.c_str(), &value, 0, 0, flags))
            {
                std::string str;
                get_language()->serialize_int(str, value);
                property_token.word_replace(str.c_str());
                changed = true;
            }
            break;
        }

        case ASTToken_t::literal_bool:
        {
            auto str   = property_token.word_to_string();
            bool value = get_language()->parse_bool_or(str, false);

            if (ImGui::Checkbox(label.c_str(), &value))
            {
                str.clear();
                get_language()->serialize_bool(str, value);
                property_token.word_replace(str.c_str());
                changed = true;
            }
            break;
        }

        default:
        {
            char buf[256];
            snprintf(buf, std::min(property_token.word_len() + 1, sizeof(buf)), "%s", property_token.word());

            if (ImGui::InputText(label.c_str(), buf, sizeof(buf), flags))
            {
                property_token.word_replace(buf);
                changed = true;
            }
            break;
        }
    }

    if ( _compact_mode )
        ImGui::PopItemWidth();

    return changed;
}

bool ASTNodePropertyView::draw_all(const std::vector<ASTNodePropertyView *>& views, ViewDetail _detail)
{
    bool changed = false;

    if ( !views.empty() )
    {
        for(auto view : views)
        {
            ImGui::SameLine();
            changed |= view->draw( _detail );
        }
    }

    return changed;
};
