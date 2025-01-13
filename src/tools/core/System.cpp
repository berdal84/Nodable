#include "System.h"
#include <cstdlib>    // for ::system
#include <thread>     // for std::thread

#include "log.h"

using namespace tools;

void System::open_url_async(std::string _URL)
{
    auto open_url = [](std::string _URL)
    {
#ifdef WIN32
        std::string command("start");
#elif __APPLE__
        std::string command("open");
#elif __linux__
        std::string command("x-www-browser"); // TODO: does not work on all distros
#else
#       error "Unknown operating system."
#endif

        std::string op = command + " " + _URL;
        auto result = ::system(op.c_str());

        if (result != 0)
        {
            LOG_ERROR( "tools::system", "Unable to open %s. Because the command %s is not available on your system.",
                       _URL.c_str(), command.c_str());
        }

        return result;
    };

    std::thread(open_url, _URL).detach();
}

void System::clear_console() /* cf: https://stackoverflow.com/questions/6486289/how-can-i-clear-console */
{
#if defined _WIN32
    const char* command = "cls";
#elif defined (__LINUX__) || defined(__gnu_linux__) || defined(__linux__) || defined (__APPLE__)
    const char* command = "reset";
#endif
    if(std::system(command)) LOG_ERROR("tools::system::console", "Unable to reset console");
}
