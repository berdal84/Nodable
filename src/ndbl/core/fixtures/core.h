#pragma once

#include "ndbl/core/NodableHeadless.h"
#include "ndbl/core/language/Nodlang.h"
#include "tools/core/FileSystem.h"
#include <exception>
#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <filesystem>

using namespace ndbl;

namespace testing
{
class Core : public Test
{
public:
    NodableHeadless app;

    void SetUp() override
    {
        app.init();

        tools::log::set_verbosity( tools::log::Verbosity_Message );
        tools::log::set_verbosity( "Parser", tools::log::Verbosity_Verbose );
    }

    void TearDown() override
    {
        app.shutdown();
    }

    std::string parse_and_serialize(const std::string &_source_code)
    {
        LOG_VERBOSE("core", "parse_and_serialize parsing \"%s\"\n", _source_code.c_str());

        // parse
        app.parse(_source_code);

        // serialize
        std::string result;
        app.serialize( result );
        LOG_VERBOSE("tools.h", "parse_and_serialize serialize_node() output is: \"%s\"\n", result.c_str());

        return result;
    }

    // load a file relative to executable directory
    std::string load_file(const char* path)
    {
        tools::Path _path = tools::Path::get_executable_path().parent_path() / path;
        std::ifstream file_stream( _path.c_str() );
        VERIFY(file_stream.is_open(), "Unable to open file!" );
        std::string program((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
        return program;
    }
    
    void log_ribbon() const
    {
        LOG_MESSAGE("fixture::core", "%s\n\n", get_language()->_state.string().c_str());
    }
};
}
