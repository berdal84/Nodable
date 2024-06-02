#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <ImGuiColorTextEdit/TextEditor.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "tools/core/geometry/Rect.h"
#include "tools/core/geometry/Vec2.h"
#include "tools/core/geometry/Vec4.h"
#include "tools/core/geometry/Space.h"
#include "tools/core/types.h"

#include "ActionManager.h"
#include "EventManager.h"
#include "tools/core/geometry/Bezier.h"

namespace tools
{
    // forward declarations
    struct Texture;

    namespace ImGuiEx
    {
        constexpr float TOOLTIP_DURATION_DEFAULT = 0.2f;
        constexpr float TOOLTIP_DELAY_DEFAULT    = 0.5f;

        struct WireStyle
        {
            Vec4 color{};
            Vec4 hover_color{};
            Vec4 shadow_color{};
            float thickness{1};
            float roundness{0.5f};
        };

        void set_debug( bool debug );

        /**
         * Draw a rounded-rectangle shadow
         * TODO: use a low cost method, this one is drawing several rectangle with modulated opacity.
        */
        extern void DrawRectShadow(
                const Vec2& _topLeftCorner,
                const Vec2& _bottomRightCorner,
                float _borderRadius = 0.0f,
                int _shadowRadius = 10,
                const Vec2& _shadowOffset = Vec2(),
                const Vec4& _shadowColor = Vec4(0.0f, 0.0f, 0.0f, 1.f));

        extern void ShadowedText(
                const Vec2& _offset,
                const Vec4& _shadowColor,
                const char *_format,
                ...);

        extern void ColoredShadowedText(
                const Vec2& _offset,
                const Vec4& _textColor,
                const Vec4& _shadowColor,
                const char *_format,
                ...);

        extern void DrawWire(
                ImDrawList* draw_list,
                const BezierCurveSegment& curve,
                const WireStyle& style);

        extern void DrawVerticalWire(
                ImDrawList *draw_list,
                const Vec2& pos0,
                const Vec2& pos1,
                const WireStyle& style);

        extern void     EndFrame();
        extern void     NewFrame();
        extern bool     BeginTooltip(float _delay = TOOLTIP_DELAY_DEFAULT, float _duration = TOOLTIP_DURATION_DEFAULT );
        extern void     EndTooltip();
        extern Rect&    EnlargeToInclude(Rect& _rect, Rect _other);
        extern void     BulletTextWrapped(const char*);
        extern Rect     GetContentRegion(Space);
        extern void     DebugRect(const Vec2& p_min, const Vec2& p_max, ImU32 col, float rounding = 0.f, ImDrawFlags flags = 0, float thickness = 1.f);
        extern void     DebugCircle(const Vec2& center, float radius, ImU32 col, int num_segments = 0, float thickness = 1.0f);
        extern void     DebugLine(const Vec2& p1, const Vec2& p2, ImU32 col, float thickness = 1.0f);
        extern void     Image(Texture*);

        template<class EventT>
        static void MenuItem(bool selected = false, bool enable = true) // Shorthand to get a given action from the manager and draw a MenuItem from it.
        {
            const IAction* action = ActionManager::get_instance().get_action_with_id(EventT::id);

            if (ImGui::MenuItem( action->label.c_str(), action->shortcut.to_string().c_str(), selected, enable))
            {
                action->trigger();
            }
        };

        template<typename ...Args>
        static void DrawHelperEx(float _alpha, const char* _format, Args... args)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _alpha);
            ImGui::Text(ICON_FA_QUESTION_CIRCLE);
            ImGui::PopStyleVar();

            if( BeginTooltip() )
            {
                ImGui::Text(_format, args...);
                EndTooltip();
            }
        }

        template<typename ...Args>
        static void DrawHelper(const char* _format, Args... args)
        { DrawHelperEx(0.25f, _format, args...); } // simple "?" test with a tooltip.

        void MultiSegmentLineBehavior(
                const std::vector<Vec2>* path,
                Rect bbox,
                float thickness);

        inline ImRect toImGui(Rect r) { return { r.min, r.max }; };
        inline ImVec2 toImGui(Vec2 v) { return { v.x, v.y }; };
        inline ImVec4 toImGui(Vec4 v) { return { v.x, v.y, v.z, v.w }; };

        float CalcSegmentHoverMinDist( float line_thickness );
        bool IsLastLineHovered();
        bool IsDraggingWire();
        void DrawPath(ImDrawList* draw_list, const std::vector<Vec2>* path, const Vec4& color, float thickness);
    };
}