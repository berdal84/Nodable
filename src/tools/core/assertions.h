#pragma once

#ifndef TOOLS_ASSERTIONS_ENABLE
#define TOOLS_ASSERTIONS_ENABLE true
#endif

#if TOOLS_ASSERTIONS_ENABLE

#define TOOLS_NOEXCEPT !TOOLS_DEBUG

// TOOLS_ASSERTIONS_ENABLE ON
//----------------------------

#include "log.h" // to flush before to assert/throw
#include <cassert>

// Exception OFF
//--------------

#if TOOLS_NOEXCEPT

#define ASSERT(expression)          assert( (expression) )
#define VERIFY(expression, message) assert( (expression) )

#else // TOOLS_NOEXCEPT

// Exception ON
//-------------

#include <exception>

#ifdef VERIFY_
static_assert(false, "VERIFY_ is reserved for tools, it should not be defined here.")
#endif

#define VERIFY_(expression, message_if_fails )\
if(!(expression)) { LOG_FLUSH(); throw std::runtime_error(message_if_fails); }

#define ASSERT(expression) VERIFY_( (expression), "Assertion failed: " #expression" is false" )
#define VERIFY(expression, message) VERIFY_( (expression), message )

#endif // !TOOLS_NOEXCEPT

// DEBUG SPECIFIC
#ifdef TOOLS_DEBUG
#define ASSERT_DEBUG_ONLY(expression) ASSERT(expression)
#else
#define ASSERT_DEBUG_ONLY(expression)
#endif

#else // TOOLS_ASSERTIONS_ENABLE

// TOOLS_ASSERTIONS_ENABLE OFF
//----------------------------

// Disable the macros completely
#define ASSERT(...)
#define VERIFY(...)

#endif // TOOLS_ASSERTIONS_ENABLE
