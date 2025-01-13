#include "App.h"

#include "tools/core/TaskManager.h"
#include "tools/core/System.h"

#include "AppView.h"
#include "Config.h"
#include "ImGuiEx.h"
#include "TextureManager.h"

using namespace tools;

void App::init()
{
    // Create and initialize a view
    auto* view = new AppView();
    view->init(this);
    m_flags |= Flag_OWNS_VIEW_MEMORY;

    // Initialize a config
    Config* config = init_config();
    m_flags |= Flag_OWNS_CONFIG_MEMORY;

    // Perform additional initialization
    init_ex(m_view, config);
}

void App::init_ex(AppView* _view, Config* _config)
{
    // Guards
    VERIFY(m_view == nullptr, "A view already exist. Did you call set_name twice?");
    VERIFY(m_config == nullptr, "A config already exist. Did you call set_name twice?");
    VERIFY(_config != nullptr, "You must provide a config");
    VERIFY(_view != nullptr, "You must provide a view");

    // Store existing data
    m_view   = _view;
    m_config = _config;

    // Initialize managers
    m_task_manager    = init_task_manager();
}

void App::shutdown()
{
    LOG_MESSAGE("tools::BaseApp", "Shutting down ...\n");

    // Optionally shutdown view
    if (m_flags & Flag_OWNS_VIEW_MEMORY )
    {
        m_view->shutdown();
    }

    // Optionally shutdown config
    if (m_flags & Flag_OWNS_CONFIG_MEMORY )
    {
        ASSERT(m_config != nullptr);
        shutdown_config(m_config);
    }

    // managers
    shutdown_task_manager(m_task_manager);

    LOG_MESSAGE("tools::BaseApp", "Shutdown OK\n");
}

void App::update()
{
    m_view->update();
    m_task_manager->update();
}

void App::draw()
{
    m_view->begin_draw();
    //
    // You can add some ImGui stuff here to debug
    //
    m_view->end_draw();
}

double App::get_time()
{
    return ImGui::GetTime();
}


Path& App::make_absolute(Path& _path)
{
    if ( _path.is_absolute() )
        return _path;
    _path = Path::get_executable_path().parent_path() / "assets" / _path;
    return _path;
}

Path App::get_absolute_asset_path(const char* _str)
{
    Path asset_path{_str};
    App::make_absolute(asset_path);
    return asset_path;
}