#include "FileSystem.h"
#include "tools/core/log.h"

#ifdef PLATFORM_DESKTOP
#include <whereami.h> // to locate executable directory
#endif

using namespace tools;

const std::filesystem::path::value_type* Path::c_str() const
{
    return m_path.c_str();
}

std::string Path::string() const
{
    return m_path.string();
}

bool Path::is_absolute() const
{
    return m_path.is_absolute();
}

bool Path::is_relative() const
{
    return m_path.is_relative();
}

Path Path::filename() const
{
    return m_path.filename();
}

bool Path::empty() const
{
    return m_path.empty();
}

Path Path::parent_path() const
{
    return m_path.parent_path();
}

bool Path::create_directories(const Path& path)
{
    return std::filesystem::create_directories(path.m_path);
}

Path Path::get_executable_path()
{
#if PLATFORM_DESKTOP
    char* path = nullptr;
    int length, dirname_length;
    length = wai_getExecutablePath(nullptr, 0, &dirname_length);
    Path result;
    if (length > 0)
    {
        path = new char[length + 1];

        if ( wai_getExecutablePath(path, length, &dirname_length) )
        {
            path[length] = '\0';
            result = path;
            TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "tools::system", "executable path: %s\n", result.c_str() );
            TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "tools::system", "  dirname: %s\n", result.parent_path().c_str() );
            TOOLS_DEBUG_LOG(TOOLS_VERBOSE, "tools::system", "  basename: %s\n", result.filename().c_str() );
        }
        else
        {
            TOOLS_LOG(TOOLS_ERROR, "tools::system", "Unable to get executable path\n");
        }
        delete[] path;
    }
    else
    {
        TOOLS_LOG(TOOLS_WARNING, "tools::system", "Unable to get executable path!\n");
    }
    return result;
#else
    return "./fake-executable";
#endif
}