#pragma once

#include "tools/core/reflection/reflection"
#include "tools/gui/ViewState.h"

#include "Condition.h"
#include "Isolation.h"
#include "types.h"
#include "tools/core/Signals.h"

namespace ndbl
{
    // forward declarations
    class File;
    class GraphView;
    class Graph;

    enum OverlayPos {
        OverlayPos_Top,
        OverlayPos_Right,
        OverlayPos_Bottom,
        OverlayPos_Left,
    };

    using OverlayType = int;
    enum OverlayType_
    {
        OverlayType_TEXT,
        OverlayType_GRAPH,
        OverlayType_COUNT
    };

    typedef struct {
        std::string label;
        std::string description;
        OverlayPos position;
    } OverlayData;

    class FileView
	{
    public:
        DECLARE_REFLECT
        explicit FileView();
        FileView(const FileView&) = delete;
		~FileView() = default;

        tools::SimpleSignal signal_text_view_changed;
        tools::SimpleSignal signal_graph_view_changed;

        void                           update(float d);
        void                           init(File& _file);
        void                           draw(float dt);
        std::string                    get_text(Isolation = Isolation_OFF)const;
        void                           set_text(const std::string&, Isolation mode = Isolation_OFF);
        TextEditor*					   get_text_editor(){ return &m_text_editor; }
        void                           set_cursor_position(const TextEditor::Coordinates& _cursorPosition) { m_text_editor.SetCursorPosition(_cursorPosition); }
        TextEditor::Coordinates        get_cursor_position()const { return m_text_editor.GetCursorPosition(); }
        void						   set_undo_buffer(TextEditor::IExternalUndoBuffer*);
        void                           draw_info_panel()const;
        void                           experimental_clipboard_auto_paste(bool);
        bool                           experimental_clipboard_auto_paste()const { return m_experimental_clipboard_auto_paste; }
        void                           clear_overlay();
        void                           push_overlay(OverlayData, OverlayType) ;
        void                           refresh_overlay(Condition condition);
        void                           draw_overlay(const char* title, const std::vector<OverlayData>& overlay_data, const tools::Rect& rect, const tools::Vec2& position);
        size_t                         size() const;
    private:
        std::array<std::vector<OverlayData>, OverlayType_COUNT> m_overlay_data;
        File*        m_file;
        GraphView*   m_graph_view;
        std::string  m_text_overlay_window_name;
        std::string  m_graph_overlay_window_name;
		TextEditor   m_text_editor;
		float        m_child1_size;
		float        m_child2_size;
        std::string  m_experimental_clipboard_curr;
        std::string  m_experimental_clipboard_prev;
        bool         m_experimental_clipboard_auto_paste;
        bool         m_is_history_dragged = false;
    };
}
