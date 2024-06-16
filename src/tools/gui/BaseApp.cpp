#include "BaseApp.h"

#include "AppView.h"
#include "Config.h"
#include "ImGuiEx.h"
#include "tools/core/TaskManager.h"
#include "tools/core/memory/memory.h"
#include "tools/core/system.h"

using namespace tools;

void BaseApp::init(AppView* _view, BaseAppFlags _flags)
{
    LOG_VERBOSE("tools::App", "init ...\n");

    m_flags = _flags;

    if ( ( m_flags & BaseAppFlag_SKIP_CONFIG) == false )
    {
        init_config();
    }

    m_task_manager = init_task_manager();
    m_texture_manager = init_texture_manager();

    if ( ( m_flags & BaseAppFlag_SKIP_VIEW) == false )
    {
        EXPECT(_view != nullptr, "View can't be null unless BaseAppFlag_SKIP_VIEW flag is ON")
        this->m_view = _view;
        this->m_view->init();
    }

    LOG_VERBOSE("tools::App", "init " OK "\n");
}

void BaseApp::shutdown()
{
    LOG_MESSAGE("tools::App", "Shutting down ...\n");

    if ( (m_flags & BaseAppFlag_SKIP_VIEW ) == false )
    {
        ASSERT(this->m_view != nullptr)
        this->m_view->shutdown();
    }

    shutdown_texture_manager();
    shutdown_task_manager();

    if ( (m_flags & BaseAppFlag_SKIP_CONFIG ) == false )
    {
        shutdown_config();
    }

    LOG_MESSAGE("tools::App", "Shutdown OK\n");
}

void BaseApp::update()
{
    LOG_VERBOSE("tools::App", "update ...\n");
    m_view->update();
    m_task_manager->update();
    LOG_VERBOSE("tools::App", "update " OK "\n");
}

void BaseApp::draw()
{
    m_view->draw();
}

double BaseApp::elapsed_time()
{
    return ImGui::GetTime();
}

std::filesystem::path BaseApp::asset_path(const std::filesystem::path& _path)
{
    EXPECT(!_path.is_absolute(), "_path is not relative, this can't be an asset")
    auto executable_dir = tools::system::get_executable_directory();
    return executable_dir / "assets" / _path;
}

std::filesystem::path BaseApp::asset_path(const char* _path)
{
    std::filesystem::path fs_path{_path};
    return  fs_path.is_absolute() ? fs_path
                                  : asset_path(fs_path);
}