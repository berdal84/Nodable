#pragma once
#include "tools/gui/App.h"
#include "tools/gui/Config.h"
#include "AppExampleView.h"

namespace tools
{
    // forward declaration
    class AppExampleView;
    
    class AppExample
    {
        friend AppExampleView;
    public:
        void        init();
        void        run();
        void        shutdown();
        void        update();
        void        draw();
        bool        should_stop() const { return m_base_app.should_stop(); };
        App*        base_app_handle() { return &m_base_app; }
    private:
        void        request_stop();

        App             m_base_app; // wrapped
        AppExampleView  m_view;
        Config*         m_config = nullptr;
    };
}