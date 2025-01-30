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
using namespace tools;

namespace testing
{
class Core : public Test
{
public:
    NodableHeadless app;

    void SetUp() override
    {
        app.init();

        set_log_verbosity( Verbosity_Message );
        set_log_verbosity( "Parser", Verbosity_Diagnostic );
    }

    void TearDown() override
    {
        app.shutdown();
    }

    std::string parse_and_serialize(const std::string &_source_code)
    {
        TOOLS_DEBUG_LOG(tools::Verbosity_Message, "core", "parse_and_serialize parsing \"%s\"\n", _source_code.c_str());

        // parse
        app.parse(_source_code);

        // serialize
        std::string result;
        app.serialize( result );
        TOOLS_DEBUG_LOG(tools::Verbosity_Message, "tools.h", "parse_and_serialize serialize_node() output is: \"%s\"\n", result.c_str());

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
        TOOLS_LOG(tools::Verbosity_Message, "fixture::core", "%s\n\n", get_language()->_state.string().c_str());
    }
};
}
