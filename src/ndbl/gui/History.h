#pragma once

#include <deque>
#include <ctime>
#include <string>
#include <memory>
#include "tools/gui/ImGuiEx.h"
#include "tools/core/log.h"
#include "tools/core/reflection/reflection"

#include "ndbl/core/Graph.h"
#include "ndbl/core/ASTNodeProperty.h"

#include "ndbl/gui/Command.h"
#include "ndbl/gui/types.h"

namespace ndbl
{
    // forward declarations
    class History;

	/* TextEditorBuffer is a class to handle TextEditor UndoRecords
	This class will catch these object using AddUndo method.
	*/
	class TextEditorBuffer : public TextEditor::IExternalUndoBuffer
    {
	public:
        void set_enable(bool _val){ m_enabled = _val; }
		void AddUndo(TextEditor::UndoRecord& _undoRecord) override;
		void set_history(History* _history) { m_history = _history; }
		void set_text_editor(TextEditor* aTextEditor) { m_Text_editor = aTextEditor;}
	private:
		TextEditor* m_Text_editor = nullptr;
		History*    m_history     = nullptr;
        bool        m_enabled     = false;
	};

    /**
     * The history is responsible for undo/redo commands.
     * It has_flags two containers to store past/future commands
     *     (past)    (now)    (future)
     * |oooooooooooooo|-------------------|
     */
	class History {
	public:
		History() = default;
        History(const History&) = delete;
		~History();

        bool is_dirty = false;

		/**
		 * Push a command and execute it.
		 * In some cases the command may not be added to the history or executed, check definition.
		 * @param _from_text_editor should not be set except if command comes from TextEditor.
		 *                          This flag is here to state legacy history mode (text based) and
		 *                          hybrid mode (Text/Graph).
		 */
		void    push_command(std::shared_ptr<AbstractCommand>, bool _from_text_editor = false);
        void    enable_text_editor(bool _val);
		void    undo();
		void    redo();
		void    clear();
		size_t  get_size()const; /** To get the set_size of the history (command count) */
		void                move_cursor(int _pos); /** Move time cursor to past (negative value) or future (positive value). */
		std::string         get_cmd_description_at(int _cmd_position);
		TextEditorBuffer*   configure_text_editor_undo_buffer(TextEditor* _textEditor); /** To get the special buffer for TextEditor */
        std::pair<int, int> get_command_id_range(); /** return the command position range. Ex: (-100, 20) if we have 100 commands to undo and 20 to redo */
    private:
        using Commands = std::deque<std::shared_ptr<AbstractCommand>>;
		TextEditorBuffer m_text_editor_buffer;
		Commands         m_past;
		Commands         m_future;
    };
}