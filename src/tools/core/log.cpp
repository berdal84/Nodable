#include "log.h"

using namespace tools;

LogState& tools::get_log_state()
{
    static LogState state{};
    return state;
}

void tools::set_log_verbosity(const std::string& category, Verbosity level)
{
    get_log_state().verbosity_by_category.insert_or_assign(category, level );
}

void tools::set_log_verbosity(Verbosity level)
{
    get_log_state().verbosity = level;
    get_log_state().verbosity_by_category.clear(); // ensure no overrides remains
}

Verbosity tools::get_log_verbosity(const std::string& category)
{
    const auto& pair = get_log_state().verbosity_by_category.find(category);
    if (pair != get_log_state().verbosity_by_category.end() )
    {
        return pair->second;
    }
    return get_log_state().verbosity;
}

void tools::flush()
{
    std::cout << std::flush;
}