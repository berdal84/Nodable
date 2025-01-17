#pragma once

#ifndef TOOLS_ASSERTIONS_ENABLE
#define TOOLS_ASSERTIONS_ENABLE true // When false, any ASSERT/VERIFY macros are disable
#endif

#if TOOLS_ASSERTIONS_ENABLE // ---------------------------------------------------------------------------

    #define TOOLS_NOEXCEPT TOOLS_RELEASE // In release, we disable exceptions and fallback on regular asserts

    #if TOOLS_NOEXCEPT // --------------------------------------------------------------------------------

        #include <cassert>
        #define ASSERT(expression)          assert( (expression) )
        #define VERIFY(expression, message) ASSERT( expression )

    #else // TOOLS_NOEXCEPT ------------------------------------------------------------------------------

        #include <exception> // for std::runtime_error
        #include "log.h"     // for tools::flush()

        #ifdef VERIFY_
            static_assert(false, "VERIFY_ is reserved for tools, it should not be defined here.")
        #endif

        #define VERIFY_(expression, message_if_fails )\
        if(!(expression)) { tools::flush(); throw std::runtime_error(message_if_fails); }

        #define ASSERT(expression) VERIFY_( (expression), "Assertion failed: " #expression" is false" )
        #define VERIFY(expression, message) VERIFY_( (expression), message )

        #endif // !TOOLS_NOEXCEPT

        // DEBUG SPECIFIC
        #ifdef TOOLS_DEBUG
        #define ASSERT_DEBUG_ONLY(expression) ASSERT(expression)
        #else
        #define ASSERT_DEBUG_ONLY(expression)
    #endif // TOOLS_NOEXCEPT -----------------------------------------------------------------------------

#else // TOOLS_ASSERTIONS_ENABLE -------------------------------------------------------------------------

    // Disable the macros completely
    #define ASSERT(...)
    #define VERIFY(...)

#endif // TOOLS_ASSERTIONS_ENABLE -----------------------------------------------------------------------
