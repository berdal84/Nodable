#pragma once
#include "Nodable.h"
#include "View.h"
#include "ImGuiColorTextEdit/TextEditor.h"
#include <mirror.h>

namespace Nodable {	

    // forwward declarations
    class File;

	class FileView : public View
	{
	public:
		FileView():m_textEditor(), m_hasChanged(false){};
		void                           init();
		bool                           draw();
		virtual bool update(){return true; };
		bool                           hasChanged() const { return this->m_hasChanged; }
		void                           setText(const std::string&);
		std::string                    getSelectedText()const;
		std::string                    getText()const;
		void                           replaceSelectedText(std::string _val);
		TextEditor*					   getTextEditor(){ return &m_textEditor; }
		void                           setTextEditorCursorPosition(const TextEditor::Coordinates& _cursorPosition) { m_textEditor.SetCursorPosition(_cursorPosition); }
		TextEditor::Coordinates        getTextEditorCursorPosition()const { return m_textEditor.GetCursorPosition(); }
		void						   setUndoBuffer(TextEditor::ExternalUndoBufferInterface*);
        void                           drawFileInfo();
        void                           experimental_clipboard_auto_paste(bool val)
        {
            m_experimental_clipboard_auto_paste = val;
            if( val ) {
                m_experimental_clipboard_prev = "";
            }
        }
        bool                           experimental_clipboard_auto_paste() { return m_experimental_clipboard_auto_paste; }
	private:
		File*        getFile();
		TextEditor   m_textEditor;
		bool         m_hasChanged;
		float        m_childSize1 = 0.3f;
		float        m_childSize2 = 0.7f;

        std::string  m_experimental_clipboard_curr;
        std::string  m_experimental_clipboard_prev;
        bool         m_experimental_clipboard_auto_paste = false;

        // reflect class using mirror
		MIRROR_CLASS(FileView)(
			MIRROR_PARENT(View)
        );
    };
}
