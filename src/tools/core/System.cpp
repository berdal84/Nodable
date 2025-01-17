#include "System.h"
#include <cstdlib>    // for ::system
#include <thread>     // for std::thread
#include "log.h"

using namespace tools;

#if PLATFORM_DESKTOP
int run_command(const char* command)
{
    int exit_code = ::system(command);
    if ( exit_code != 0 )
    {
        TOOLS_LOG(TOOLS_ERROR, "tools::system", "Command failed: %s", command);
    }
    return exit_code;
};

void System::open_url_async(std::string url)
{
    std::string command = "x-www-browser " + url; // TODO: does not work on all distros
    std::thread thread( run_command, command.c_str() );
    thread.detach();
}

void System::clear_console() /* cf: https://stackoverflow.com/questions/6486289/how-can-i-clear-console */
{
    if( std::system("clear") )
    {
        TOOLS_LOG(TOOLS_ERROR, "System", "Unable to reset console");
    }
}

#elif PLATFORM_WEB
#include <emscripten.h>

EM_JS(void, call_clear_console, (), {
  alert('call_clear_console not implemented yet');
  throw 'all done';
});

EM_JS(void, call_open_url, (), {
  alert('call_open_url not implemented yet');
  throw 'all done';
});

void System::open_url_async(std::string url)
{
    call_open_url();
}

void System::clear_console() /* cf: https://stackoverflow.com/questions/6486289/how-can-i-clear-console */
{
    call_clear_console();
}

#endif
