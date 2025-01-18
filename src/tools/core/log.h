#pragma once

#include "./string.h"
#include "format.h"
#include <chrono>
#include <ctime>
#include <deque>
#include <iostream>
#include <map>
#include <string>

#define TOOLS_COLOR_DEFAULT     "\033[0m"
#define TOOLS_COLOR_BLACK       "\033[30m"        /* Black */
#define TOOLS_COLOR_RED         "\033[31m"        /* Red */
#define TOOLS_COLOR_GREEN       "\033[32m"        /* Green */
#define TOOLS_COLOR_YELLOW      "\033[33m"        /* Yellow */
#define TOOLS_COLOR_BLUE        "\033[34m"        /* Blue */
#define TOOLS_COLOR_MAGENTA     "\033[35m"        /* Magenta */
#define TOOLS_COLOR_CYAN        "\033[36m"        /* Cyan */
#define TOOLS_COLOR_WHITE       "\033[37m"        /* White */
#define TOOLS_COLOR_BOLDBLACK   "\033[1m\033[30m" /* Bold Black */
#define TOOLS_COLOR_BOLDRED     "\033[1m\033[31m" /* Bold Red */
#define TOOLS_COLOR_BOLDGREEN   "\033[1m\033[32m" /* Bold Green */
#define TOOLS_COLOR_BOLDYELLOW  "\033[1m\033[33m" /* Bold Yellow */
#define TOOLS_COLOR_BOLDBLUE    "\033[1m\033[34m" /* Bold Blue */
#define TOOLS_COLOR_BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define TOOLS_COLOR_BOLDCYAN    "\033[1m\033[36m" /* Bold Cyan */
#define TOOLS_COLOR_BOLDWHITE   "\033[1m\033[37m" /* Bold White */

#define TOOLS_KO   TOOLS_COLOR_BOLDRED "[KO]" TOOLS_COLOR_DEFAULT // red   colored "[KO]" string.
#define TOOLS_OK TOOLS_COLOR_BOLDGREEN "[OK]" TOOLS_COLOR_DEFAULT // green colored "[OK]" string.

#define TOOLS_LOG(...) tools::log( __VA_ARGS__ )

#ifdef TOOLS_DEBUG
#   define TOOLS_DEBUG_LOG(...)         TOOLS_LOG( __VA_ARGS__ )
#   define TOOLS_LOG_MESSAGE_MAX_COUNT  500000
#   define TOOLS_LOG_VERBOSITY_DEFAULT  tools::Verbosity_Diagnostic
#   define TOOLS_LOG_MESSAGE_MEMORY_MAX 400*1000*1000
#else
#   define TOOLS_DEBUG_LOG(...)         /* TOOLS_DEBUG_LOG disabled when TOOLS_DEBUG is false */
#   define TOOLS_LOG_MESSAGE_MAX_COUNT  1000
#   define TOOLS_LOG_VERBOSITY_DEFAULT  tools::Verbosity_Message
#   define TOOLS_LOG_MESSAGE_MEMORY_MAX 2*1000*1000
#endif

namespace tools
{
    // Different verbosity levels a message can have
    typedef int Verbosity;
    enum Verbosity_: int
    {
        Verbosity_Error      = 0, // lowest level (always logged)
        Verbosity_Warning    = 1,
        Verbosity_Message    = 2,
        Verbosity_Diagnostic = 3, // highest level (rarely logged)

        Verbosity_COUNT,
        Verbosity_FilterAll  = -1
    };

    struct MessageData
    {
        using clock_t = std::chrono::time_point<std::chrono::system_clock>;

        const char* category = "";                        // short category name (ex: "Game", "App", etc.)
        Verbosity verbosity{TOOLS_LOG_VERBOSITY_DEFAULT}; // verbosity level
        string512 text{};                                 // message content
        clock_t   date{std::chrono::system_clock::now()}; // printed at date
    };

    struct LogState
    {
        Verbosity                        verbosity = TOOLS_LOG_VERBOSITY_DEFAULT;
        std::map<std::string, Verbosity> verbosity_by_category = {};
        std::deque<MessageData>          messages = {};
    };

    LogState&        get_log_state();
    void             set_log_verbosity(const char* category, Verbosity level); // Set verbosity level for a given category
    void             set_log_verbosity(Verbosity level); // override verbosity globally
    Verbosity        get_log_verbosity(const char* category);
    static Verbosity get_log_verbosity() { return get_log_state().verbosity; }
    void             flush(); // Ensure all messages have been printed out
    bool             show_log_message(const MessageData&, Verbosity filter); // return true if messages needs to be displayed depending on filter and global verbosity

    template<typename...Args>
    void log(Verbosity verbosity, const char* category, const char* format, Args... args) // print a message like "[time|verbosity|category] message"
    {
        struct VerbosityInfo
        {
            const char* label;
            const char* color;
        };

        constexpr VerbosityInfo verbosity_info[Verbosity_COUNT] = {
            { "ERROR"     , TOOLS_COLOR_RED     }, // Verbosity_Error
            { "WARNING"   , TOOLS_COLOR_MAGENTA }, // Verbosity_Warning
            { "MESSAGE"   , TOOLS_COLOR_DEFAULT }, // Verbosity_Message
            { "DIAGNOSTIC", TOOLS_COLOR_DEFAULT }, // Verbosity_Diagnostic
        };

        MessageData message;
        message.verbosity = verbosity;
        message.category  = category;
        // text prefix
        message.text.append_fmt(
                "[%s|%s|%s] ",
                format::time_point_to_string(message.date).c_str(),
                verbosity_info[verbosity].label,
                category
                );
        // text body
        message.text.append_fmt(format, args...);

        // print if allowed
        if ( message.verbosity <= get_log_verbosity(category) )
        {
            // Print colored content
            printf("%s%s%s",
                verbosity_info[verbosity].color,
                message.text.c_str(),
                TOOLS_COLOR_DEFAULT);

            // add to logs
            get_log_state().messages.emplace_front(message);
        }

        // Constraint the queue to have a limited size
        if ( get_log_state().messages.size() > TOOLS_LOG_MESSAGE_MAX_COUNT )
        {
            get_log_state().messages.resize( TOOLS_LOG_MESSAGE_MAX_COUNT / 2 );
        }
    }

    static_assert(
        TOOLS_LOG_MESSAGE_MAX_COUNT * (sizeof(MessageData)) < TOOLS_LOG_MESSAGE_MEMORY_MAX,
        "tools' log messages can go above limit");
}
