#pragma once

#include "geometry/BoxShape2D.h"
#include "geometry/Rect.h"
#include "geometry/Space.h"
#include "ImGuiEx.h" // ImGui with extensions

namespace tools
{
    /**
     * This class is not supposed to be used as-is to draw a view, it has to be wrapped.
     * See examples in NodeView or SlotView
     */
	struct ViewState
	{
        typedef int Flags;
        enum Flags_
        {
            Flags_None     = 0,
            Flags_Pinned   = 1 << 0,
            Flags_Selected = 1 << 1,
            Flags_Visible  = 1 << 2,
            Flags_Hovered  = 1 << 3,
        };

		ViewState(Flags flags = Flags_Visible);

        bool                 pinned() const              { return _flags & Flags_Pinned; }
        bool                 selected() const            { return _flags & Flags_Selected;   }
        bool                 visible() const             { return _flags & Flags_Visible;  }
        bool                 hovered() const             { return _flags & Flags_Hovered;  }
        void                 set_pinned(bool b = true)   { _flags = (_flags & ~Flags_Pinned)   | ( b * Flags_Pinned   ) ;}
        void                 set_selected(bool b = true) { _flags = (_flags & ~Flags_Selected) | ( b * Flags_Selected ) ;}
        void                 set_visible(bool b = true)  { _flags = (_flags & ~Flags_Visible)  | ( b * Flags_Visible  ) ;}
        void                 set_hovered(bool b = true)  { _flags = (_flags & ~Flags_Hovered)  | ( b * Flags_Hovered  ) ;}
    private:
        Flags      _flags = Flags_None;
    };
}
