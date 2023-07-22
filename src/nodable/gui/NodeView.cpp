#include "NodeView.h"

#include <cmath>                  // for sinus
#include <algorithm>              // for std::max
#include <vector>

#include "fw/core/math.h"
#include "fw/core/reflection/registration.h"

#include "core/Graph.h"
#include "core/InvokableComponent.h"
#include "core/LiteralNode.h"
#include "core/Scope.h"
#include "core/VariableNode.h"
#include "core/language/Nodlang.h"

#include "Config.h"
#include "Event.h"
#include "Nodable.h"
#include "NodeConnector.h"
#include "PropertyConnector.h"
#include "PropertyView.h"
#include "NodeViewConstraint.h"

constexpr ImVec2 NODE_VIEW_DEFAULT_SIZE(10.0f, 35.0f);

using namespace ndbl;

REGISTER
{
    fw::registration::push_class<NodeView>("NodeView")
        .extends<Component>()
        .extends<fw::View>();
}

NodeView*          NodeView::s_selected                        = nullptr;
NodeView*          NodeView::s_dragged                         = nullptr;
NodeViewDetail     NodeView::s_view_detail                     = NodeViewDetail::Default;
const float        NodeView::s_property_input_size_min           = 10.0f;
const ImVec2 NodeView::s_property_input_toggle_button_size(10.0, 25.0f);
std::vector<NodeView*> NodeView::s_instances;

NodeView::NodeView()
        : Component()
        , fw::View()
        , m_position(500.0f, -1.0f)
        , m_size(NODE_VIEW_DEFAULT_SIZE)
        , m_opacity(1.0f)
        , m_expanded(true)
        , m_force_property_inputs_visible(false)
        , pinned(false)
        , m_border_radius(5.0f)
        , m_border_color_selected(1.0f, 1.0f, 1.0f)
        , m_exposed_this_property_view(nullptr)
        , m_edition_enable(true)
        , m_apply_constraints(true)
{
    NodeView::s_instances.push_back(this);
}

NodeView::~NodeView()
{
    // delete PropertyViews
    for ( auto& pair: m_exposed_properties ) delete pair.second;

    // deselect
    if ( s_selected == this ) s_selected = nullptr;

    // delete NodeConnectors
    for( auto& conn : m_successors ) delete conn;
    for( auto& conn : m_predecessors ) delete conn;

    // Erase instance in static vector
    auto found = std::find( s_instances.begin(), s_instances.end(), this);
    assert(found != s_instances.end() );
    s_instances.erase(found);
}

std::string NodeView::get_label()
{
    Node* node = get_owner();

    if (s_view_detail == NodeViewDetail::Minimalist )
    {
        // I always add an ICON_FA at the beginning of any node label string (encoded in 4 bytes)
        return m_short_label;
    }
    return m_label;
}

void NodeView::expose(Property * _property)
{
    auto property_view = new PropertyView(_property, this);

    if ( _property == get_owner()->as_property )
    {
        property_view->m_out->m_display_side = PropertyConnector::Side::Left; // force to be displayed on the left
        m_exposed_this_property_view = property_view;
    }
    else
    {
        if (_property->get_allowed_connection() == Way_In)
        {
            m_exposed_input_only_properties.push_back(property_view);
        }
        else
        {
            m_exposed_out_or_inout_properties.push_back(property_view);
        }
    }

    m_exposed_properties.insert({_property, property_view});
}

void NodeView::set_owner(Node *_node)
{
    Component::set_owner(_node);

    const Config& config = Nodable::get_instance().config;
    std::vector<Property *> not_exposed;

    // 1. Expose properties (make visible)
    //------------------------------------

    //  We expose first the properties which allows input connections

    for(Property * each_property : _node->props.by_index())
    {
        if (each_property->get_visibility() == Visibility::Always && each_property->allows_connection(Way_In) )
        {
            expose(each_property);
        }
        else
        {
            not_exposed.push_back(each_property);
        }
    }

    // Then we expose node which allows output connection (if they are not yet exposed)
    for (auto& property : not_exposed)
    {
        if (property->get_visibility() == Visibility::Always && property->allows_connection(Way_Out))
        {
            expose(property);
        }
    }

    if ( auto this_property = _node->as_property )
    {
        expose(this_property);
    }

    // 2. NodeConnectors
    //------------------

    // add a successor connector per successor slot
    const size_t successor_max_count = _node->successors.get_limit();
    for(size_t index = 0; index < successor_max_count; ++index )
    {
        m_successors.push_back(new NodeConnector(*this, Way_Out, index, successor_max_count));
    }

    // add a single predecessor connector if node can be connected in this way
    if(_node->predecessors.get_limit() != 0)
        m_predecessors.push_back(new NodeConnector(*this, Way_In));

    // 4. Listen to connection/disconnections
    //---------------------------------------

    _node->on_edge_added.connect(
        [this](Node* _other_node, Edge_t _edge )
        {
            NodeView* _other_node_view = _other_node->components.get<NodeView>();
            switch ( _edge )
            {
                case Edge_t::IS_CHILD_OF:
                    children.add(_other_node_view ); break;
                case Edge_t::IS_INPUT_OF:
                    inputs.add(_other_node_view ); break;
                case Edge_t::IS_OUTPUT_OF:
                    outputs.add(_other_node_view ); break;
                case Edge_t::IS_SUCCESSOR_OF:
                    successors.add(_other_node_view ); break;
                case Edge_t::IS_PREDECESSOR_OF:
                    FW_ASSERT(false); /* NOT HANDLED */break;
            }
        });

    _node->on_edge_removed.connect(
    [this](Node* _other_node, Edge_t _edge )
        {
            NodeView* _other_node_view = _other_node->components.get<NodeView>();
            switch ( _edge )
            {
                case Edge_t::IS_CHILD_OF:
                    children.remove(_other_node_view ); break;
                case Edge_t::IS_INPUT_OF:
                    inputs.remove(_other_node_view ); break;
                case Edge_t::IS_OUTPUT_OF:
                    outputs.remove(_other_node_view ); break;
                case Edge_t::IS_SUCCESSOR_OF:
                    successors.remove(_other_node_view ); break;
                case Edge_t::IS_PREDECESSOR_OF:
                    FW_ASSERT(false); /* NOT HANDLED */break;
            }
        });

    // update label
    update_labels_from_name(_node);
    _node->on_name_change.connect([&](Node* _node) {
        update_labels_from_name(_node);
    });
}

void NodeView::update_labels_from_name(Node* _node)
{
    // Label

    m_label.clear();
    m_short_label.clear();
    if ( auto variable = fw::cast<const VariableNode>(_node))
    {
        m_label += variable->get_value()->get_type()->get_name();
        m_label += " ";
    }
    m_label += _node->name;

    // Short label
    constexpr size_t label_max_length = 10;
    if (m_label.size() > label_max_length)
    {
        m_short_label = m_label.substr(0, label_max_length) + "..";
    }
    else
    {
        m_short_label = m_label;
    }
}

void NodeView::set_selected(NodeView* _view)
{
    if( s_selected == _view ) return;

    if( s_selected)
    {
        Event event{ EventType_node_view_deselected };
        event.node.view = s_selected;
        fw::EventManager::get_instance().push_event((fw::Event&)event);
    }

    if( _view )
    {
        Event event{ EventType_node_view_selected };
        event.node.view = _view;
        fw::EventManager::get_instance().push_event((fw::Event&)event);
    }

	s_selected = _view;
}

NodeView* NodeView::get_selected()
{
	return s_selected;
}

void NodeView::start_drag(NodeView* _view)
{
	if(PropertyConnector::get_gragged() == nullptr) // Prevent dragging node while dragging connector
		s_dragged = _view;
}

bool NodeView::is_any_dragged()
{
	return get_dragged() != nullptr;
}

NodeView* NodeView::get_dragged()
{
	return s_dragged;
}

bool NodeView::is_selected(NodeView* _view)
{
	return s_selected == _view;
}

const PropertyView* NodeView::get_property_view(const Property * _property)const
{
    auto found = m_exposed_properties.find(_property);
    if( found == m_exposed_properties.end() ) return nullptr;
    return found->second;
}

void NodeView::set_position(ImVec2 _position, fw::Space origin)
{
    switch (origin)
    {
        case fw::Space_Local: m_position = _position - position_offset_user; break;
        case fw::Space_Screen: m_position = _position - position_offset_user - m_screen_space_content_region.GetTL(); break;
        default:
            FW_EXPECT(false, "OriginRef_ case not handled, cannot compute perform set_position(...)")
    }
}

ImVec2 NodeView::get_position(fw::Space origin, bool round) const
{
    // compute position depending on space
    ImVec2 result = m_position + position_offset_user;
    if (origin == fw::Space_Screen) result += m_screen_space_content_region.GetTL();

    // return rounded or not if needed
    if(round) return fw::math::round(result);
    return result;
}

void NodeView::translate(ImVec2 _delta, bool _recurse)
{
    ImVec2 current_local_position = get_position(fw::Space_Local);
    set_position( current_local_position + _delta, fw::Space_Local);

	if ( !_recurse ) return;

    for(auto eachInput : get_owner()->inputs )
    {
        NodeView* eachInputView = eachInput->components.get<NodeView>();
        if ( eachInputView && !eachInputView->pinned && eachInputView->should_follow_output(this) )
        {
                eachInputView->translate(_delta, true);
        }
    }
}

void NodeView::arrange_recursively(bool _smoothly)
{
    std::vector<NodeView*> views;

    for (auto each_input_view: inputs)
    {
        if (each_input_view->should_follow_output(this))
        {
            views.push_back(each_input_view);
            each_input_view->arrange_recursively();
        }
    }

    for (auto each_child_view: children)
    {
        views.push_back(each_child_view);
        each_child_view->arrange_recursively();
    }

    // Force an update of input nodes with a delta time extra high
    // to ensure all nodes will be well placed in a single call (no smooth moves)
    if ( !_smoothly )
    {
        update(float(1000));
    }

    pinned = false;
    position_offset_user = {};
}

bool NodeView::update(float _deltaTime)
{
    if(m_opacity != 1.0f) fw::math::lerp(m_opacity, 1.0f, 10.0f * _deltaTime);

    apply_forces(_deltaTime, false);
	return true;
}

bool NodeView::draw_implem()
{
	bool      changed  = false;
	auto      node     = get_owner();
	Config&   config   = Nodable::get_instance().config;

    FW_ASSERT(node != nullptr);

    // Draw Node connectors (in background)
    bool is_connector_hovered = false;
    {
        ImColor color        = config.ui_node_nodeConnectorColor;
        ImColor hoveredColor = config.ui_node_nodeConnectorHoveredColor;

        auto draw_and_handle_evt = [&](NodeConnector *connector)
        {
            NodeConnector::draw(connector, color, hoveredColor, m_edition_enable);
            is_connector_hovered |= ImGui::IsItemHovered();
        };

        std::for_each(m_predecessors.begin(), m_predecessors.end(), draw_and_handle_evt);
        std::for_each(m_successors.begin()  , m_successors.end()  , draw_and_handle_evt);

    }

	// Begin the window
	//-----------------
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_opacity);
	const auto halfSize = m_size / 2.0;
    ImVec2 node_screen_center_pos = get_position(fw::Space_Screen, true);
    const ImVec2 &node_top_left_corner = node_screen_center_pos - halfSize;
    ImGui::SetCursorScreenPos(node_top_left_corner); // start from th top left corner
	ImGui::PushID(this);


	// Draw the background of the Group
    auto border_color = is_selected(this) ? m_border_color_selected : get_color(ColorType_Border);
    DrawNodeRect(
            node_top_left_corner, node_top_left_corner + m_size,
            get_color(ColorType_Fill), get_color(ColorType_BorderHighlights), get_color(ColorType_Shadow), border_color,
            is_selected(this), 5.0f, config.ui_node_padding);

    // Add an invisible just on top of the background to detect mouse hovering
	ImGui::SetCursorScreenPos(node_top_left_corner);
	ImGui::InvisibleButton("node", m_size);
    ImGui::SetItemAllowOverlap();
    ImGui::SetCursorScreenPos(node_top_left_corner + config.ui_node_padding); // top left corner + padding in x and y.
	ImGui::SetCursorPosX( ImGui::GetCursorPosX() + config.ui_node_propertyConnectorRadius); // add + space for "this" left connector
    bool is_node_hovered = ImGui::IsItemHovered();

	// Draw the window content
	//------------------------

    ImGui::BeginGroup();
        std::string label = get_label().empty() ? " " : get_label();                        // ensure a 1 char width, to be able to grab it
        if ( !m_expanded )
        {
            // symbolize the fact node view is not expanded
            //abel.insert(0, "<<");
            label.append(" " ICON_FA_OBJECT_GROUP);
        }
        fw::ImGuiEx::ShadowedText(ImVec2(1.0f), get_color(ColorType_BorderHighlights), label.c_str()); // text with a lighter shadow (incrust effect)

        ImGui::SameLine();

        ImGui::BeginGroup();

        // draw properties
        auto draw_property_lambda = [&](PropertyView* view) {
            ImGui::SameLine();
            changed |= draw_property(view);
        };
        std::for_each(m_exposed_input_only_properties.begin(), m_exposed_input_only_properties.end(), draw_property_lambda);
        std::for_each(m_exposed_out_or_inout_properties.begin(), m_exposed_out_or_inout_properties.end(), draw_property_lambda);


        ImGui::EndGroup();
        ImGui::SameLine();

        ImGui::SetCursorPosX( ImGui::GetCursorPosX() + config.ui_node_padding * 2.0f);
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + config.ui_node_padding );
    ImGui::EndGroup();

    // Ends the Window
    //----------------
    ImVec2 node_top_right_corner = ImGui::GetCursorScreenPos();
    m_size.x = std::max( 1.0f, std::ceil(ImGui::GetItemRectSize().x));
    m_size.y = std::max( 1.0f, std::ceil(node_top_right_corner.y - node_top_left_corner.y ));

    // Draw Property in/out connectors
    {
        float radius      = config.ui_node_propertyConnectorRadius;
        ImColor color     = config.ui_node_nodeConnectorColor;
        ImColor borderCol = config.ui_node_borderColor;
        ImColor hoverCol  = config.ui_node_nodeConnectorHoveredColor;

        if ( m_exposed_this_property_view )
        {
            PropertyConnector::draw(m_exposed_this_property_view->m_out, radius, color, borderCol, hoverCol, m_edition_enable);
            is_connector_hovered |= ImGui::IsItemHovered();
        }

        for( auto& propertyView : m_exposed_input_only_properties )
        {
            PropertyConnector::draw(propertyView->m_in, radius, color, borderCol, hoverCol, m_edition_enable);
            is_connector_hovered |= ImGui::IsItemHovered();
        }

        for( auto& propertyView : m_exposed_out_or_inout_properties )
        {
            if ( propertyView->m_in)
            {
                PropertyConnector::draw(propertyView->m_in, radius, color, borderCol, hoverCol, m_edition_enable);
                is_connector_hovered |= ImGui::IsItemHovered();
            }

            if ( propertyView->m_out)
            {
                PropertyConnector::draw(propertyView->m_out, radius, color, borderCol, hoverCol, m_edition_enable);
                is_connector_hovered |= ImGui::IsItemHovered();
            }
        }
    }

    // Contextual menu (right click)
    if ( is_node_hovered && !is_connector_hovered && ImGui::IsMouseReleased(1))
    {
        ImGui::OpenPopup("NodeViewContextualMenu");
    }

    if (ImGui::BeginPopup("NodeViewContextualMenu"))
    {
        if( ImGui::MenuItem("Arrange"))
        {
            this->arrange_recursively();
        }

        ImGui::MenuItem("Pinned", "", &pinned, true);

		if ( ImGui::MenuItem("Expanded", "", &m_expanded, true) )
        {
		    set_expanded(m_expanded);
        }

        ImGui::Separator();

        if( ImGui::Selectable("Delete", !m_edition_enable ? ImGuiSelectableFlags_Disabled : ImGuiSelectableFlags_None))
        {
            node->flagged_to_delete = true;
        }

        ImGui::EndPopup();
    }

	// Selection by mouse (left or right click)
	if ( is_node_hovered && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)))
	{
        set_selected(this);
    }

	// Mouse dragging
	if (get_dragged() != this)
	{
		if(get_dragged() == nullptr && ImGui::IsMouseDown(0) && is_node_hovered && ImGui::IsMouseDragPastThreshold(0))
        {
            start_drag(this);
        }
	}
	else if ( ImGui::IsMouseReleased(0))
	{
        start_drag(nullptr);
	}		

	// Collapse on/off
	if( is_node_hovered && ImGui::IsMouseDoubleClicked(0))
	{
        expand_toggle();
	}

	ImGui::PopStyleVar();
	ImGui::PopID();

	get_owner()->dirty |= changed;

    m_is_hovered = is_node_hovered || is_connector_hovered;

	return changed;
}
void NodeView::DrawNodeRect(ImVec2 rect_min, ImVec2 rect_max, ImColor color, ImColor border_highlight_col, ImColor shadow_col, ImColor border_col, bool selected, float border_radius, float padding)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw the rectangle under everything
    fw::ImGuiEx::DrawRectShadow(rect_min, rect_max, border_radius, 4, ImVec2(1.0f), shadow_col);
    draw_list->AddRectFilled(rect_min, rect_max, color, border_radius);
    draw_list->AddRect(rect_min + ImVec2(1.0f), rect_max, border_highlight_col, border_radius);
    draw_list->AddRect(rect_min, rect_max, border_col, border_radius);

    // Draw an additional blinking rectangle when selected
    if (selected)
    {
        auto alpha   = sin(ImGui::GetTime() * 10.0F) * 0.25F + 0.5F;
        float offset = 4.0f;
        draw_list->AddRect(rect_min - ImVec2(offset), rect_max + ImVec2(offset), ImColor(1.0f, 1.0f, 1.0f, float(alpha) ), border_radius + offset, ~0, offset / 2.0f);
    }

}

bool NodeView::draw_property(PropertyView *_view)
{
    bool      show;
    bool      changed      = false;
    Property* property     = _view->m_property;
    const fw::type* owner_type = property->get_owner()->get_type();

    /*
     * Handle input visibility
     */
    if ( _view->m_touched )  // in case user touched it, we keep the current state
    {
        show = _view->m_showInput;
    }
    else if( s_view_detail == NodeViewDetail::Exhaustive )
    {
        show = true;
    }
    else if( fw::extends<LiteralNode>(property->get_owner()) )
    {
        show = true;                               // we always show literal´s
    }
    else if( fw::extends<VariableNode>(property->get_owner()) )
    {
        show = property->get_variant()->is_defined();
    }
    else if( !property->has_input_connected() )
    {
        show = property->get_variant()->is_defined();   // we always show a defined unconnected property
    }
    else if (property->get_type()->is_ptr() )
    {
        show = property->is_connected_to_variable();
    }
    else if ( property->is_connected_to_variable() )
    {
        show = true;
    }
    else
    {
        show = property->get_variant()->is_defined();
    }
    _view->m_showInput = show;

    // input
    float input_size = NodeView::s_property_input_toggle_button_size.x;

    if ( _view->m_showInput )
    {
        bool limit_size = !property->get_type()->is<bool>();

        if ( limit_size )
        {
            // try to draw an as small as possible input field
            std::string str;
            if (property->is_connected_to_variable())
            {
                str = property->get_connected_variable()->name;
            }
            else
            {
                str = property->convert_to<std::string>();
            }
            input_size = 5.0f + std::max(ImGui::CalcTextSize(str.c_str()).x, NodeView::s_property_input_size_min);
            ImGui::PushItemWidth(input_size);
        }
        changed = NodeView::draw_input(property, nullptr);

        if ( limit_size )
        {
            ImGui::PopItemWidth();
        }
    }
    else
    {
        ImGui::Button("", NodeView::s_property_input_toggle_button_size);

        if ( fw::ImGuiEx::BeginTooltip() )
        {
            ImGui::Text("%s (%s)",
                        property->get_name().c_str(),
                        property->get_type()->get_fullname().c_str());
            fw::ImGuiEx::EndTooltip();
        }

        if ( ImGui::IsItemClicked(0) )
        {
            _view->m_showInput = !_view->m_showInput;
            _view->m_touched = true;
        }
    }

    // compute center position
    ImVec2 pos = ImGui::GetItemRectMin() ;
    fw::ImGuiEx::DebugCircle(pos, 2.5f, ImColor(0,0,0));
    pos += ImGui::GetItemRectSize() * 0.5f;

    // memorize
    _view->m_position = pos;

    return changed;
}

bool NodeView::draw_input(Property *_property, const char *_label)
{
    bool  changed = false;
    Node* node    = _property->get_owner();

    // Create a label (everything after ## will not be displayed)
    std::string label;
    if ( _label != nullptr )
    {
        label.append(_label);
    }
    else
    {
        label.append("##" + _property->get_name());
    }

    auto inputFlags = ImGuiInputTextFlags_None;

    if( _property->has_input_connected() && _property->is_connected_to_variable() ) // if is a ref to a variable, we just draw variable name
    {
        char str[255];
        auto* variable = fw::cast<const VariableNode>(_property->get_input()->get_owner());
        snprintf(str, 255, "%s", variable->name.c_str() );

        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4) variable->components.get<NodeView>()->get_color(ColorType_Fill) );
        ImGui::InputText(label.c_str(), str, 255, inputFlags);
        ImGui::PopStyleColor();

    }
    else if( !_property->get_variant()->is_initialized() )
    {
        ImGui::LabelText(label.c_str(), "uninitialized!");
    }
    else
    {
        /* Draw the property */
        const fw::type* t = _property->get_type();

        if(t->is<i16_t>() )
        {
            auto i16 = (i16_t)*_property;

            if (ImGui::InputInt(label.c_str(), &i16, 0, 0, inputFlags ) && !_property->has_input_connected())
            {
                _property->set(i16);
                changed |= true;
            }
        }
        else if(t->is<double>() )
        {
            auto d = (double)*_property;

            if (ImGui::InputDouble(label.c_str(), &d, 0.0F, 0.0F, "%g", inputFlags ) && !_property->has_input_connected())
            {
                _property->set(d);
                changed |= true;
            }
        }
        else if(t->is<std::string>() )
        {
            char str[255];
            snprintf(str, 255, "%s", ((std::string)*_property).c_str() );

            if ( ImGui::InputText(label.c_str(), str, 255, inputFlags) && !_property->has_input_connected() )
            {
                _property->set( std::string(str) );
                changed |= true;
            }
        }
        else if(t->is<bool>() )
        {
            std::string checkBoxLabel = _property->get_name();

            auto b = (bool)*_property;

            if (ImGui::Checkbox(label.c_str(), &b ) && !_property->has_input_connected() )
            {
                _property->set(b);
                changed |= true;
            }
        }
        else
        {
            auto property_as_string = _property->get_variant()->convert_to<std::string>();
            ImGui::Text( "%s", property_as_string.c_str());
        }

        /* If value is hovered, we draw a tooltip that print the source expression of the value*/
        if (fw::ImGuiEx::BeginTooltip())
        {
            std::string buffer;
            Nodlang::get_instance().serialize(buffer, _property);
            ImGui::Text("%s", buffer.c_str() );
            fw::ImGuiEx::EndTooltip();
        }
    }

    return changed;
}

bool NodeView::is_inside(NodeView* _nodeView, ImRect _rect) {
	return _rect.Contains(_nodeView->get_rect());
}

void NodeView::draw_as_properties_panel(NodeView *_view, bool *_show_advanced)
{
    Node*       node             = _view->get_owner();
    const float labelColumnWidth = ImGui::GetContentRegionAvail().x / 2.0f;

    auto drawProperty = [&](Property * _property)
    {
        // label (<name> (<way> <type>): )
        ImGui::SetNextItemWidth(labelColumnWidth);
        ImGui::Text(
                "%s (%s, %s): ",
                _property->get_name().c_str(),
                WayToString(_property->get_allowed_connection()).c_str(),
                _property->get_type()->get_fullname().c_str());

        ImGui::SameLine();
        ImGui::Text("(?)");
        if ( fw::ImGuiEx::BeginTooltip() )
        {
            const auto variant = _property->get_variant();
            ImGui::Text("initialized: %s,\n"
                        "defined:     %s,\n"
                        "Source token:\n"
                        "%s\n",
                        variant->is_initialized() ? "true" : "false",
                        variant->is_defined()     ? "true" : "false",
                        _property->token.json().c_str()
                        );
            fw::ImGuiEx::EndTooltip();
        }
        // input
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        bool edited = NodeView::draw_input(_property, nullptr);
        _property->get_owner()->dirty |= edited;

    };

    ImGui::Text("Name:       \"%s\"" , node->name.c_str());
    ImGui::Text("Class:      %s"     , node->get_type()->get_name());

    // Draw exposed input properties
    ImGui::Separator();
    ImGui::Text("Input(s):" );
    ImGui::Separator();
    ImGui::Indent();
    if( _view->m_exposed_input_only_properties.empty() )
    {
        ImGui::Text("None.");
        ImGui::Separator();
    }
    else
    {
        for (auto& eachView : _view->m_exposed_input_only_properties )
        {
            drawProperty(eachView->m_property);
            ImGui::Separator();
        }
    }
    ImGui::Unindent();

    // Draw exposed output properties
    ImGui::Text("Output(s):" );
    ImGui::Separator();
    ImGui::Indent();
    if( _view->m_exposed_out_or_inout_properties.empty() )
    {
        ImGui::Text("None.");
        ImGui::Separator();
    }
    else
    {
        for (auto& eachView : _view->m_exposed_out_or_inout_properties )
        {
            drawProperty(eachView->m_property);
            ImGui::Separator();
        }
    }
    ImGui::Unindent();

    if ( ImGui::TreeNode("Debug") )
    {
        // Draw exposed output properties
        if( ImGui::TreeNode("Other Properties") )
        {
            drawProperty(_view->m_exposed_this_property_view->m_property);
            ImGui::TreePop();
        }

        // Components
        if( ImGui::TreeNode("Components") )
        {
            for (auto &pair : node->components)
            {
                Component *component = pair.second;
                ImGui::BulletText("%s", component->get_type()->get_name());
            }
            ImGui::TreePop();
        }

        if( ImGui::TreeNode("Slots") )
        {
            auto draw_slots = [](const char *label, const Slots<Node *> &slots)
            {
                if( ImGui::TreeNode(label) )
                {
                    if (!slots.empty())
                    {
                        for (auto each : slots.content())
                        {
                            ImGui::BulletText("- %s", each->name.c_str());
                        }
                    }
                    else
                    {
                        ImGui::BulletText("None");
                    }
                    ImGui::TreePop();
                }
            };

            draw_slots("Inputs:"      , node->inputs);
            draw_slots("Outputs:"     , node->outputs);
            draw_slots("Predecessors:", node->predecessors);
            draw_slots("Successors:"  , node->successors);
            draw_slots("Children:"    , node->children);

            ImGui::TreePop();
        }

        // m_apply_constraints
        ImGui::Separator();
        if( ImGui::TreeNode("Constraints") )
        {
            ImGui::Checkbox("Apply", &_view->m_apply_constraints);
            int i = 0;
            for(auto& constraint : _view->m_constraints)
            {
                constraint.draw_view();
            }
            ImGui::TreePop();
        }

        // Scope specific:
        ImGui::Separator();
        if (Scope* scope = node->components.get<Scope>())
        {
            if( ImGui::TreeNode("Variables") )
            {
                auto vars = scope->get_variables();
                for (auto eachVar : vars)
                {
                    ImGui::BulletText("%s: %s", eachVar->name.c_str(), eachVar->get_value()->convert_to<std::string>().c_str());
                }
                ImGui::TreePop();
            }
        }

        if( ImGui::TreeNode("Misc:") )
        {
            // dirty state
            ImGui::Separator();
            bool b = _view->get_owner()->dirty;
            ImGui::Checkbox("Is dirty ?", &b);

            // Parent graph
            {
                std::string parentName = "NULL";

                if (node->parent_graph) {
                    parentName = "Graph";
                    parentName.append(node->parent_graph->is_dirty() ? " (dirty)" : "");

                }
                ImGui::Text("Parent graph is \"%s\"", parentName.c_str());
            }

            // Parent
            ImGui::Separator();
            {
                std::string parentName = "NULL";

                if (node->parent) {
                    parentName = node->parent->name + (node->parent->dirty ? " (dirty)" : "");
                }
                ImGui::Text("Parent node is \"%s\"", parentName.c_str());
            }
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
    ImGui::Separator();
}

void NodeView::constraint_to_rect(NodeView* _view, ImRect _rect)
{
	
	if ( !NodeView::is_inside(_view, _rect)) {

		_rect.Expand(ImVec2(-2, -2)); // shrink

		auto nodeRect = _view->get_rect();

		auto newPos = _view->get_position(fw::Space_Local, true);

		auto left  = _rect.Min.x - nodeRect.Min.x;
		auto right = _rect.Max.x - nodeRect.Max.x;
		auto up    = _rect.Min.y - nodeRect.Min.y;
		auto down  = _rect.Max.y - nodeRect.Max.y;

		     if ( left > 0 )  nodeRect.TranslateX(left);
		else if ( right < 0 ) nodeRect.TranslateX(right);
			 
			 if ( up > 0 )    nodeRect.TranslateY(up);
		else if ( down < 0 )  nodeRect.TranslateY(down);

        _view->set_position(nodeRect.GetCenter(), fw::Space_Local);
	}

}

bool NodeView::is_exposed(const Property *_property)const
{
    return m_exposed_properties.find(_property) != m_exposed_properties.end();
}

void NodeView::set_view_detail(NodeViewDetail _viewDetail)
{
    NodeView::s_view_detail = _viewDetail;

    for( auto& eachView : NodeView::s_instances)
    {
        for( auto& eachPair : eachView->m_exposed_properties )
        {
            PropertyView* propertyView = eachPair.second;
            propertyView->reset();
        }
    }
}

ImRect NodeView::get_rect(bool _recursively, bool _ignorePinned, bool _ignoreMultiConstrained, bool _ignoreSelf) const
{
    if( !_recursively)
    {
        ImVec2 local_position = get_position(fw::Space_Local);
        ImRect rect{local_position, local_position};
        rect.Expand(m_size);

        return rect;
    }

    ImRect result_rect( ImVec2(std::numeric_limits<float>::max()), ImVec2(-std::numeric_limits<float>::max()) );

    if ( !_ignoreSelf && m_is_visible )
    {
        ImRect self_rect = get_rect(false);
        fw::ImGuiEx::EnlargeToInclude(result_rect, self_rect);
    }

    auto enlarge_to_fit_all = [&](NodeView* _view)
    {
        if( !_view) return;

        if ( _view->m_is_visible && !(_view->pinned && _ignorePinned) && _view->should_follow_output(this) )
        {
            ImRect child_rect = _view->get_rect(true, _ignorePinned, _ignoreMultiConstrained);
            fw::ImGuiEx::EnlargeToInclude(result_rect, child_rect);
        }
    };

    std::for_each(children.begin(), children.end(), enlarge_to_fit_all);
    std::for_each(inputs.begin()  , inputs.end()  , enlarge_to_fit_all);

    fw::ImGuiEx::DebugRect(result_rect.Min, result_rect.Max, IM_COL32( 0, 255, 0, 255 ),4 );

    return result_rect;
}

void NodeView::clear_constraints()
{
    m_constraints.clear();
}

void NodeView::add_constraint(NodeViewConstraint& _constraint)
{
    m_constraints.push_back(std::move(_constraint));
}

void NodeView::apply_constraints(float _dt)
{
    if( !m_apply_constraints ) return;

    for (NodeViewConstraint& eachConstraint : m_constraints)
    {
        eachConstraint.apply(_dt);
    }
}

void NodeView::add_force_to_translate_to(ImVec2 desiredPos, float _factor, bool _recurse)
{
    ImVec2 delta(desiredPos - m_position);
    auto factor = std::max(0.0f, _factor);
    add_force(delta * factor, _recurse);
}

void NodeView::add_force(ImVec2 force, bool _recurse)
{
    m_forces_sum += force;

    if ( _recurse )
    {
        for ( NodeView* each_input_view: inputs )
        {
            if (!each_input_view->pinned && each_input_view->should_follow_output(this))
            {
                each_input_view->add_force(force, _recurse);
            }
        }
    }
}

void NodeView::apply_forces(float _dt, bool _recurse)
{
    float magnitude = std::sqrt(m_forces_sum.x * m_forces_sum.x + m_forces_sum.y * m_forces_sum.y );

    constexpr float magnitude_max  = 1000.0f;
    const float     friction       = fw::math::lerp (0.0f, 0.5f, magnitude / magnitude_max);
    const ImVec2 avg_forces_sum      = (m_forces_sum + m_last_frame_forces_sum) * 0.5f;

    translate( avg_forces_sum * ( 1.0f - friction) * _dt , _recurse);

    m_last_frame_forces_sum = avg_forces_sum;
    m_forces_sum            = ImVec2();
}

void NodeView::translate_to(ImVec2 desiredPos, float _factor, bool _recurse)
{
    ImVec2 delta(desiredPos - m_position);

    bool isDeltaTooSmall = delta.x * delta.x + delta.y * delta.y < 0.01f;
    if (!isDeltaTooSmall)
    {
        auto factor = std::min(1.0f, _factor);
        translate(delta * factor, _recurse);
    }
}

ImRect NodeView::get_rect(
        const std::vector<const NodeView*>* _views,
        bool _recursive,
        bool _ignorePinned,
        bool _ignoreMultiConstrained)
{
    ImRect rect(ImVec2(std::numeric_limits<float>::max()), ImVec2(-std::numeric_limits<float>::max()) );

    for (auto eachView : *_views)
    {
        if ( eachView->m_is_visible )
        {
            auto each_rect = eachView->get_rect(_recursive, _ignorePinned, _ignoreMultiConstrained);
            fw::ImGuiEx::EnlargeToInclude(rect, each_rect);
        }
    }
    return rect;
}

void NodeView::set_expanded_rec(bool _expanded)
{
    set_expanded(_expanded);
    for( NodeView* each_child_view: children )
    {
        each_child_view->set_expanded_rec(_expanded);
    }
}

void NodeView::set_expanded(bool _expanded)
{
    m_expanded = _expanded;
    set_inputs_visible(_expanded, true);
    set_children_visible(_expanded, true);
}

bool NodeView::should_follow_output(const NodeView* output) const
{
    if ( outputs.empty()) return true;
    return outputs[0] == output;
}

void NodeView::set_inputs_visible(bool _visible, bool _recursive)
{
    for( NodeView* each_input_view: inputs )
    {
        if( _visible || (outputs.empty() || each_input_view->should_follow_output(this)) )
        {
            if ( _recursive && each_input_view->m_expanded ) // propagate only if expanded
            {
                each_input_view->set_children_visible(_visible, true);
                each_input_view->set_inputs_visible(_visible, true);
            }
            each_input_view->set_visible(_visible);
        }
    }
}

void NodeView::set_children_visible(bool _visible, bool _recursive)
{
    for( auto each_child_view : children )
    {
        if( _visible || (outputs.empty() || each_child_view->should_follow_output(this)) )
        {
            if ( _recursive && each_child_view->m_expanded) // propagate only if expanded
            {
                each_child_view->set_children_visible(_visible, true);
                each_child_view->set_inputs_visible(_visible, true);
            }
            each_child_view->set_visible(_visible);
        }
    }
}

void NodeView::expand_toggle()
{
    set_expanded(!m_expanded);
}

NodeView* NodeView::substitute_with_parent_if_not_visible(NodeView* _view, bool _recursive)
{
    if( !_view->is_visible() )
    {
        return _view;
    }

    Node* parent = _view->get_owner()->parent;
    if ( !parent )
    {
        return _view;
    }

    NodeView* parent_view = parent->components.get<NodeView>();
    if ( !parent_view )
    {
        return _view;
    }

    if (  _recursive )
    {
        return substitute_with_parent_if_not_visible(parent_view, _recursive);
    }

    return parent_view;
}

void NodeView::expand_toggle_rec()
{
    return set_expanded_rec(!m_expanded);
}

ImRect NodeView::get_screen_rect()
{
    ImVec2 half_size = m_size / 2.0f;
    ImVec2 screen_pos = get_position(fw::Space_Screen, false);
    return {screen_pos - half_size, screen_pos + half_size};
}