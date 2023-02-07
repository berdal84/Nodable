#include <fw/gui/Event.h>
#include <algorithm>
#include <cmath>
#include <fw/gui/ImGuiEx.h>
#include "fw/core/Log.h"
#include "fw/core/assertions.h"

using namespace fw;

bool    ImGuiEx::s_is_in_a_frame            = false;
bool    ImGuiEx::s_is_any_tooltip_open      = false;
float   ImGuiEx::s_tooltip_duration_default = 0.2f;
float   ImGuiEx::s_tooltip_delay_default    = 0.5f;
float   ImGuiEx::s_tooltip_delay_elapsed    = 0.0f;

vec2 ImGuiEx::CursorPosToScreenPos(vec2 _position)
{
    return _position + ToScreenPosOffset();
}

vec2 ImGuiEx::ToScreenPosOffset()
{
    return ImGui::GetCursorScreenPos() - ImGui::GetCursorPos();
}


void ImGuiEx::DrawRectShadow (vec2 _topLeftCorner, vec2 _bottomRightCorner, float _borderRadius, int _shadowRadius, vec2 _shadowOffset, ImColor _shadowColor)
{
    vec2 itemRectMin(_topLeftCorner.x + _shadowOffset.x, _topLeftCorner.y + _shadowOffset.y);
    vec2 itemRectMax(_bottomRightCorner.x + _shadowOffset.x, _bottomRightCorner.y + _shadowOffset.y);
    vec4 color       = _shadowColor;
    color.w /= _shadowRadius;
    auto borderRadius  = _borderRadius;

    // draw N concentric rectangles.
    for(int i = 0; i < _shadowRadius; i++)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        draw_list->AddRectFilled(itemRectMin, itemRectMax, ImColor(color), borderRadius);

        itemRectMin.x -= 1.0f;
        itemRectMin.y -= 1.0f;

        itemRectMax.x += 1.0f;
        itemRectMax.y += 1.0f;

        borderRadius += 1.0f;
    }
}

void ImGuiEx::ShadowedText(vec2 _offset, ImColor _shadowColor, const char* _format, ...)
{
    // draw first the shadow
    auto p = ImGui::GetCursorPos();
    ImGui::SetCursorPos(vec2(p.x + _offset.x, p.y + _offset.y));

    va_list args;
    va_start(args, _format);
    ImGui::TextColored(_shadowColor, _format, args);
    ImGui::SetCursorPos(p);
    ImGui::Text(_format, args);
    va_end(args);
}

void ImGuiEx::ColoredShadowedText(vec2 _offset, ImColor _textColor, ImColor _shadowColor, const char* _format, ...)
{
    // draw first the shadow
    auto p = ImGui::GetCursorPos();
    ImGui::SetCursorPos(vec2(p.x + _offset.x, p.y + _offset.y));

    va_list args;
    va_start(args, _format);
    ImGui::TextColored(_shadowColor, _format, args);
    ImGui::SetCursorPos(p);
    ImGui::TextColored(_textColor, _format, args);
    va_end(args);
}

void ImGuiEx::DrawVerticalWire(
        ImDrawList *draw_list,
        vec2 pos0,
        vec2 pos1,
        ImColor color,
        ImColor shadowColor,
        float thickness,
        float roundness)
{
    // Compute tangents
    float roundedDist = std::abs(pos1.y - pos0.y) * roundness;

    vec2 cp0_fill(pos0.x , pos0.y + roundedDist);
    vec2 cp1_fill(pos1.x , pos1.y - roundedDist);

    vec2 pos_shadow_offset(1.f, 1.f);
    vec2 pos0_shadow(pos0 + pos_shadow_offset);
    vec2 pos1_shadow(pos1 + pos_shadow_offset);
    vec2 cp0_shadow(pos0_shadow.x , pos0_shadow.y + roundedDist * 1.05f);
    vec2 cp1_shadow(pos1_shadow.x , pos1_shadow.y - roundedDist * 0.95f);

    // shadow
    draw_list->AddBezierCurve( pos0_shadow, cp0_shadow, cp1_shadow, pos1_shadow, shadowColor, thickness);
    // fill
    draw_list->AddBezierCurve( pos0, cp0_fill, cp1_fill, pos1, color, thickness);
}

void ImGuiEx::DrawHorizontalWire(
        ImDrawList *draw_list,
        vec2 pos0,
        vec2 pos1,
        ImColor color,
        ImColor shadowColor,
        float thickness,
        float roundness)
{

    // Compute tangents
    float dist = std::max(std::abs(pos1.y - pos0.y), 200.0f);

    vec2 cp0(pos0.x + dist * roundness , pos0.y );
    vec2 cp1(pos1.x - dist * roundness , pos1.y );

    // draw bezier curve
    vec2 shadowOffset(1.0f, 2.0f);
    draw_list->AddBezierCurve(  pos0 + shadowOffset,
                                cp0  + shadowOffset,
                                cp1  + shadowOffset,
                                pos1 + shadowOffset,
                                shadowColor,
                                thickness); // shadow

    draw_list->AddBezierCurve(pos0, cp0, cp1, pos1, color, thickness); // fill
}

ImRect& ImGuiEx::EnlargeToInclude(ImRect& _rect, ImRect _other)
{
    if( _other.Min.x < _rect.Min.x) _rect.Min.x = _other.Min.x;
    if( _other.Min.y < _rect.Min.y) _rect.Min.y = _other.Min.y;
    if( _other.Max.x > _rect.Max.x) _rect.Max.x = _other.Max.x;
    if( _other.Max.y > _rect.Max.y) _rect.Max.y = _other.Max.y;

    return _rect;
}

bool ImGuiEx::BeginTooltip(float _delay, float _duration)
{
    if ( !ImGui::IsItemHovered() ) return false;

    NDBL_EXPECT(s_is_in_a_frame, "Did you forgot to call ImGuiEx::BeginFrame/EndFrame ?");

    s_is_any_tooltip_open     = true;
    s_tooltip_delay_elapsed   += ImGui::GetIO().DeltaTime;

    float fade = 0.f;
    if (s_tooltip_delay_elapsed >= _delay )
    {
        fade = (s_tooltip_delay_elapsed - _delay) / _duration;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);
    ImGui::BeginTooltip();

    return true;
}

void ImGuiEx::EndTooltip()
{
    ImGui::EndTooltip();
    ImGui::PopStyleVar(); // ImGuiStyleVar_Alpha
}

void ImGuiEx::EndFrame()
{
    NDBL_EXPECT(s_is_in_a_frame, "ImGuiEx::BeginFrame/EndFrame mismatch");
    if( !s_is_any_tooltip_open )
    {
        s_tooltip_delay_elapsed    = 0.f;
    }
    s_is_in_a_frame = false;
}

void ImGuiEx::BeginFrame()
{

    NDBL_EXPECT(!s_is_in_a_frame, "ImGuiEx::BeginFrame/EndFrame mismatch");
    s_is_in_a_frame = true;
    s_is_any_tooltip_open = false;
}

void ImGuiEx::MenuItemBindedToEvent(uint16_t type, bool selected, bool enable)
{
    auto binded_evt = BindedEventManager::get_event(type);
    if (ImGui::MenuItem( binded_evt.label.c_str(),   binded_evt.shortcut.to_string().c_str(), selected, enable))
    {
        EventManager::push_event(binded_evt.event_t);
    }
}

void ImGuiEx::BulletTextWrapped(const char* str)
{
    ImGui::Bullet(); ImGui::SameLine();
    ImGui::TextWrapped("%s", str);
}