#include "WireView.h"
#include "NodeView.h"
#include "Wire.h"
#include "Node.h"

#include <imgui/imgui.h>
#include <Settings.h>

using namespace Nodable;

bool WireView::draw()
{
    auto settings = Settings::GetCurrent();
    auto wire = getOwner()->as<Wire>();
	NODABLE_ASSERT(wire != nullptr);

	// Update fill color depending on current state 
	ImVec4 stateColors[Wire::State_COUNT] = {ImColor(1.0f, 0.0f, 0.0f), ImColor(0.8f, 0.8f, 0.8f)};
	setColor(ColorType_Fill, &stateColors[wire->getState()]);

	// draw the wire
	auto source = wire->getSource();
	auto target = wire->getTarget();

	if ( source && target )
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

	    // Compute start and end point
	    auto sourceNode 	= source->getOwner()->as<Node>();
		auto targetNode 	= target->getOwner()->as<Node>();

		if (!sourceNode->hasComponent<View>() || // in case of of the node have no view we can't draw the wire.
			!targetNode->hasComponent<View>() )
			return false;

	    auto sourceView = sourceNode->getComponent<NodeView>();
	    auto targetView = targetNode->getComponent<NodeView>();

		if (!sourceView->isVisible() || !targetView->isVisible() ) // in case of of the node have hidden view we can't draw the wire.
			return false;

		ImVec2 pos0 = sourceView->getConnectorPosition(wire->getSource(), Way_Out);
		ImVec2 pos1 = targetView->getConnectorPosition(wire->getTarget(), Way_In);

        WireView::DrawVerticalWire(draw_list, pos0, pos1, getColor(ColorType_Fill), getColor(ColorType_Shadow),
                                   settings->ui.wire.bezier.thickness,
                                   settings->ui.wire.bezier.roundness);

        // dot at the output position
        draw_list->AddCircleFilled(pos0, settings->ui.nodes.connectorRadius, sourceView->getColor(ColorType_Fill));
        draw_list->AddCircle      (pos0, settings->ui.nodes.connectorRadius, sourceView->getColor(ColorType_Border));

        ImVec2 arrowSize(8.0f, 12.0f);
        if (settings->ui.wire.displayArrows)
        {
            // Arrow at the input position
            draw_list->AddLine(ImVec2(pos1.x - arrowSize.x, pos1.y + arrowSize.y/2.0f), pos1, getColor(ColorType_Fill), settings->ui.wire.bezier.thickness);
            draw_list->AddLine(ImVec2(pos1.x - arrowSize.x, pos1.y - arrowSize.y/2.0f), pos1, getColor(ColorType_Fill), settings->ui.wire.bezier.thickness);
        }
        else
        {
            // dot at the input position
            draw_list->AddCircleFilled(pos1, settings->ui.nodes.connectorRadius, targetView->getColor(ColorType_Fill));
            draw_list->AddCircle      (pos1, settings->ui.nodes.connectorRadius, targetView->getColor(ColorType_Border));
        }
    }

    return false;
}

void WireView::DrawVerticalWire(
        ImDrawList *draw_list,
        ImVec2 pos0,
        ImVec2 pos1,
        ImColor color,
        ImColor shadowColor,
        float thickness,
        float roundness)
{
    // Compute tangents
    float dist = std::abs(pos1.y - pos0.y);

    ImVec2 cp0(pos0.x , pos0.y + dist * roundness);
    ImVec2 cp1(pos1.x , pos1.y - dist * roundness);

    // draw bezier curve
    ImVec2 shadowOffset(1.0f, 2.0f);
    draw_list->AddBezierCurve(  pos0 + shadowOffset,
                                cp0  + shadowOffset,
                                cp1  + shadowOffset,
                                pos1 + shadowOffset,
                                shadowColor,
                                thickness); // shadow

    draw_list->AddBezierCurve(pos0, cp0, cp1, pos1, color, thickness); // fill
}

void WireView::DrawHorizontalWire(
        ImDrawList *draw_list,
        ImVec2 pos0,
        ImVec2 pos1,
        ImColor color,
        ImColor shadowColor,
        float thickness,
        float roundness)
{

    // Compute tangents
    float dist = std::max(std::abs(pos1.y - pos0.y), 200.0f);

    ImVec2 cp0(pos0.x + dist * roundness , pos0.y );
    ImVec2 cp1(pos1.x - dist * roundness , pos1.y );

    // draw bezier curve
    ImVec2 shadowOffset(1.0f, 2.0f);
    draw_list->AddBezierCurve(  pos0 + shadowOffset,
                                cp0  + shadowOffset,
                                cp1  + shadowOffset,
                                pos1 + shadowOffset,
                                shadowColor,
                                thickness); // shadow

    draw_list->AddBezierCurve(pos0, cp0, cp1, pos1, color, thickness); // fill
}
