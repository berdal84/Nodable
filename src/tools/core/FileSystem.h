#pragma once
#include <filesystem>
#include <string>

namespace tools
{
    // Wraps a std::filesystem object as a multi-platform interface (std::string under linux is super permissive, but it does not compiles on MSVC..)
    class Path
    {
    public:

        Path()
        : m_path()
        {}
        Path(const char* str)
        : m_path(str)
        {}
        Path(const std::filesystem::path& str)
        : m_path(str)
        {}

        const std::filesystem::path::value_type* c_str()const; // Not compatible with "const char*" on WIN32
        std::string  string()const;
        bool         is_absolute() const;
        bool         is_relative() const;
        Path         filename() const;
        bool         empty() const;
        Path         parent_path() const;

        static bool exists(const Path& path)
        { return std::filesystem::exists(path.m_path); };

        Path& operator=(const char* other)
        { m_path = other; return *this; }

        Path& operator/(const Path& other)
        { m_path /= other.m_path; return *this; }

        static bool  create_directories(const Path&);
        static Path  get_executable_path();            // Get the executable directory absolute path
    private:
        std::filesystem::path m_path;
    };

}