#pragma once
#include "tools/gui/AppView.h"

namespace tools
{
    // Forward declarations
    class AppExample;

    class AppExampleView
    {
    public:
        void            init(AppExample* _app);
        void            shutdown();
        void            update();
        void            draw();
        AppView* base_view_handle() { return &m_base_view; }
        void            _draw_splashscreen_content();
        void            _reset_layout();
    private:
        AppView         m_base_view; // wrapped
        AppExample*     m_app       = nullptr; // NOT owned
    };
}