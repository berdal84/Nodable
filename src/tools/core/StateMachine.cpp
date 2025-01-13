#include "StateMachine.h"
#include "tools/core/assertions.h"

using namespace tools;

StateMachine::~StateMachine()
{
    for(auto& [hash, state] : m_state)
        delete state;
}

void StateMachine::set_default_state(const char* name)
{
    State* state = get_state(name);
    VERIFY(state != nullptr, "Can't find state");
    VERIFY(m_default_state == nullptr, "This method must be called once");
    m_default_state = state;
}

void StateMachine::tick()
{
    if ( !started() )
        return;

#if TOOLS_DEBUG_STATE_MACHINE
    if ( ImGui::Begin("StateMachine Debug"))
    {
        ImGui::Text("Current State:     %s (id %u)", m_current_state->name, m_current_state->id);
        ImGui::End();
    }
#endif

    ASSERT(m_current_state != nullptr);
    m_current_state->delegate[OnTick].call();

    // Early return if no transition is found
    if ( m_next_state == nullptr )
        return;

    // Switch to next_state
    m_current_state->delegate[OnLeave].call();
    m_next_state->delegate[OnEnter].call();
    m_current_state = m_next_state;
    m_next_state    = nullptr;
}

void StateMachine::start()
{
    VERIFY( !started(), "StateMachine is already started");
    m_current_state = m_default_state;
    m_current_state->delegate[OnEnter].call();
}

void StateMachine::stop()
{
    VERIFY( started(), "StateMachine is not started");
    m_current_state->delegate[OnLeave].call();
    m_current_state = nullptr;
}

State* StateMachine::add_state(const char* _name)
{
    auto* state = new State();
    state->name = _name;
    add_state(state);
    return state;
}

void StateMachine::add_state(State* state)
{
    const auto key = Hash::hash( state->name );
    const auto& [it, success] = m_state.emplace( key, state);
    VERIFY(success, "State name already exists");
}

void StateMachine::set_next_state(State* state)
{
    VERIFY(m_next_state == nullptr, "Can't change twice within a single tick");
    m_next_state = state;
}

void StateMachine::exit_state()
{
    VERIFY(m_current_state != m_default_state, "Default state can't be exited!");
    set_next_state(m_default_state);
}

State *StateMachine::get_state(const char *name)
{
    auto it = m_state.find( Hash::hash(name) );
    if( it != m_state.end())
        return it->second;
    return nullptr;
}

void StateMachine::change_state(const char *name)
{
    State* state = get_state(name);
    VERIFY(state != nullptr, "Unable to find state");
    set_next_state(state);
}