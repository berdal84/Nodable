#pragma once

#include <future>
#include <memory>
#include <string>

#include "tools/gui/App.h"

#include "Config.h"
#include "types.h"

namespace ndbl
{
    // forward declarations
    class Nodlang;
    class ASTNodeFactory;
    class NodableView;
    class File;

    class Nodable
    {
	public:
        // Common

        void            init();
        void            run();
        void            update();
        void            draw();
        void            shutdown();
        bool            should_stop() const;
        NodableView*    get_view() const;
        tools::App*     get_base_app_handle() { return &m_base_app; }
        void            _do_frame();

        // Files

        File*           open_asset_file(const tools::Path&);
        File*           open_file(const tools::Path&);
        File*           new_file();
        void            save_file(File*) const;
        void            set_current_file(File*);
        void            save_file_as(File*, const tools::Path&) const;
        File*           add_file(File*);
        void            close_file(File*);
        File*           get_current_file() { return m_current_file; };
        bool            is_current(const File* _file) const { return m_current_file == _file; }
        const std::vector<File*>&
                        get_files() const { return m_loaded_files; }
        bool            has_files() const { return !m_loaded_files.empty(); }
        void            reset_current_graph();
        static Nodable* instance() { return s_instance; }
    private:
        static Nodable*    s_instance;
        tools::App         m_base_app;
        NodableView*       m_view              = nullptr;
        Config*            m_config            = nullptr;
        File*              m_current_file      = nullptr;
        Nodlang*           m_language          = nullptr;
        ASTNodeFactory*    m_node_factory      = nullptr;
        u8_t               m_untitled_file_count = 0;
        std::vector<File*> m_loaded_files;
        std::vector<File*> m_flagged_to_delete_file;
    };
}
