#include "ActionManager.h"

#include <SDL_keyboard.h>
#include <future>
#include <thread>

#include "tools/core/TaskManager.h"
#include "tools/core/assertions.h"
#include "tools/core/log.h"

using namespace tools;

ActionManager* g_action_manager = nullptr;

ActionManager* tools::init_action_manager()
{
    VERIFY(g_action_manager == nullptr, "Cannot be called twice");
    g_action_manager = new ActionManager();
    return g_action_manager;
}

ActionManager* tools::get_action_manager()
{
    VERIFY(g_action_manager != nullptr, "event manager can't be found. Did you call set_name ?");
    return g_action_manager;
}

void tools::shutdown_action_manager(ActionManager* _action_manager)
{
    ASSERT(_action_manager == g_action_manager);
    ASSERT(g_action_manager != nullptr);
    delete g_action_manager;
    g_action_manager = nullptr;
}

ActionManager::~ActionManager()
{
    for(auto* action : m_actions )
        delete action;
}

const IAction* ActionManager::get_action_with_id(EventID id) const
{
    auto found = m_actions_by_id.find(id);
    if ( found == m_actions_by_id.end() )
    {
        string128 str;
        str.append_fmt("Unable to find an action bound to EventId %i\n", id);
        VERIFY(false, str.c_str() );
    }
    return found->second;
}

const std::vector<IAction*>& ActionManager::get_actions() const
{
    return m_actions;
}

void ActionManager::add_action( IAction* _action )// Add a new action (can be triggered via shortcut)
{
    m_actions.push_back( _action );
    m_actions_by_id.insert(std::pair{_action->event_id, _action});
    TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "ActionManager", "Action '%s' bound to the event_id %i\n", _action->label.c_str(), _action->event_id);
}

std::string Shortcut::to_string() const
{
    std::string result;

    if (mod & KMOD_CTRL) result += "Ctrl + ";
    if (mod & KMOD_ALT)  result += "Alt + ";
    if (key)             result += SDL_GetKeyName(key);
    if (!description.empty()) result += description;

    return result;
}
