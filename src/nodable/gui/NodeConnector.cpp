#include "NodeConnector.h"

#include "core/Graph.h"
#include "core/Node.h"

#include "Config.h"
#include "Event.h"
#include "NodeView.h"
#include "Nodable.h"

using namespace ndbl;

const NodeConnector*     NodeConnector::s_dragged   = nullptr;
const NodeConnector*     NodeConnector::s_hovered   = nullptr;
const NodeConnector*     NodeConnector::s_focused   = nullptr;

void NodeConnector::draw(const NodeConnector *_connector, const ImColor &_color, const ImColor &_hoveredColor, bool _editable)
{
    constexpr float rounding = 6.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImRect      rect      = _connector->get_rect();
    ImVec2      rect_size = rect.GetSize();

    // Return early if rectangle cannot be draw.
    // TODO: Find why size can be zero more (more surprisingly) nan.
    if(rect_size.x == 0.0f || rect_size.y == 0.0f || std::isnan(rect_size.x) || std::isnan(rect_size.y) ) return;

    ImDrawCornerFlags cornerFlags = _connector->m_way == Way_Out ? ImDrawCornerFlags_Bot : ImDrawCornerFlags_Top;

    auto cursorScreenPos = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(rect.GetTL());
    ImGui::PushID(_connector);
    ImGui::InvisibleButton("###", rect.GetSize());
    ImGui::PopID();
    ImGui::SetCursorScreenPos(cursorScreenPos);

    ImColor color = ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) ? _hoveredColor : _color;
    draw_list->AddRectFilled(rect.Min, rect.Max, color, rounding, cornerFlags );
    draw_list->AddRect(rect.Min, rect.Max, ImColor(50,50, 50), rounding, cornerFlags );
    fw::ImGuiEx::DebugRect(rect.Min, rect.Max, ImColor(255,0, 0, 127), 0.0f );

    // behavior
    auto connectedNode = _connector->get_connected_node();
    if ( _editable && connectedNode && ImGui::BeginPopupContextItem() )
    {
        if ( ImGui::MenuItem(ICON_FA_TRASH " Disconnect"))
        {
            Event event{};
            event.type = EventType_node_connector_disconnected;
            event.node_connectors.src = _connector;
            event.node_connectors.dst = nullptr;
            fw::EventManager::get_instance().push_event((fw::Event&)event);
        }

        ImGui::EndPopup();
    }

    if ( ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) )
    {
        if ( ImGui::IsMouseDown(0) && !is_dragging() && !NodeView::is_any_dragged())
        {
            if ( _connector->m_way == Way_Out)
            {
                const Slots<Node*>& successors = _connector->get_node()->successors;
                if (successors.size() < successors.get_limit() )
                {
                    start_drag(_connector);
                }
            }
            else
            {
                const Slots<Node*>& predecessors = _connector->get_node()->predecessors;
                if ( predecessors.empty() )
                {
                    start_drag(_connector);
                }
            }
        }

        s_hovered = _connector;
    }
    else if ( s_hovered == _connector )
    {
        s_hovered = nullptr;
    }
}

ImRect NodeConnector::get_rect() const
{
    Config& config         = Nodable::get_instance().config;

    // pick a corner
    ImVec2   left_corner = m_way == Way_In ?
            m_node_view.get_screen_rect().GetTL() : m_node_view.get_screen_rect().GetBL();

    // compute connector size
    ImVec2 size(
            std::min(config.ui_node_connector_width,  m_node_view.get_size().x),
            std::min(config.ui_node_connector_height, m_node_view.get_size().y));
    ImRect rect(left_corner, left_corner + size);
    rect.Translate(ImVec2(size.x * float(m_index), -rect.GetSize().y * 0.5f) );
    rect.Expand(ImVec2(- config.ui_node_connector_padding, 0.0f));

    return rect;
}

ImVec2 NodeConnector::get_pos() const
{
    return get_rect().GetCenter();
}

bool NodeConnector::share_parent_with(const NodeConnector *other) const
{
    return get_node() == other->get_node();
}

void NodeConnector::dropped(const NodeConnector *_left, const NodeConnector *_right)
{
    NodeConnectorEvent evt{};
    evt.type = EventType_node_connector_dropped;
    evt.src = _left;
    evt.dst = _right;
    fw::EventManager::get_instance().push_event((fw::Event&)evt);
}

Node* NodeConnector::get_connected_node() const
{
    auto node = get_node();

    if ( m_way == Way_In )
    {
        if (node->predecessors.size() > m_index )
        {
            return node->predecessors[m_index];
        }
    }
    else if (node->successors.size() > m_index )
    {
        return node->successors[m_index];
    }
    return nullptr;

}

Node* NodeConnector::get_node()const
{
    return m_node_view.get_owner();
}
