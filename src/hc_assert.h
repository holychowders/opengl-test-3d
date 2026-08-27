#pragma once

#include <stdio.h>

// DEBUG_BREAK()
#if defined(_MSC_VER)
    #include <intrin.h>
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
    #define DEBUG_BREAK() __builtin_debugtrap()
#elif defined(__GNUC__)
    #include <signal.h>
    #if defined(SIGTRAP)
        #define DEBUG_BREAK() raise(SIGTRAP)
    #else
        #define DEBUG_BREAK() __builtin_trap()
    #endif
#else
    #include <signal.h>
    #if defined(SIGTRAP)
        #define DEBUG_BREAK() raise(SIGTRAP)
    #else
        #include <stdlib.h>
        #define DEBUG_BREAK() abort()
    #endif
#endif

// ASSERT()
#define ASSERT(expr)                                                                                                                                 \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            fprintf(stderr, "ASRT: (%s) failed\n      Location: %s, line %d, function %s", (#expr), __FILE__, __LINE__, __FUNCTION__);               \
            DEBUG_BREAK();                                                                                                                           \
        }                                                                                                                                            \
    } while (0)

// ASSERT_MSG() - Raise with an additional message
#define ASSERT_MSG(expr, msg)                                                                                                                        \
    do {                                                                                                                                             \
        if (!(expr)) {                                                                                                                               \
            fprintf(stderr,                                                                                                                          \
                    "ASRT: %s\n      Failure: %s\n      Location: %s, line %d, function %s",                                                         \
                    (msg),                                                                                                                           \
                    (#expr),                                                                                                                         \
                    __FILE__,                                                                                                                        \
                    __LINE__,                                                                                                                        \
                    __FUNCTION__);                                                                                                                   \
            DEBUG_BREAK();                                                                                                                           \
        }                                                                                                                                            \
    } while (0)
//fprintf(stderr, "ASRT: Line %d of %s in %s() (%s)\n      Failure: %s\n      Comment: %s\n", __LINE__, __FILE_NAME__, __FUNCTION__,  __FILE__, (#expr), (msg));\
//fprintf(stderr, "ASRT [%s]: (%s) failed\n     Info: %s:%d@%s (%s)",(msg), (#expr), __FILE_NAME__, __LINE__, __FUNCTION__,  __FILE__);\
//fprintf(stderr, "ASRT [%s]: %s:%d@%s (%s)\n     Comment: %s",(#expr), __FILE_NAME__, __LINE__, __FUNCTION__,  __FILE__, (msg));\
//fprintf(stderr, "ASRT [%s]: %s:%d@%s (%s)\n     Failure: %s",(msg), __FILE_NAME__, __LINE__, __FUNCTION__,  __FILE__, (#expr));\

// ASSERT_MSG() - Raise with no expression and just a message
#define ASSERT_RAISE(msg)                                                                                                                            \
    do {                                                                                                                                             \
        fprintf(stderr, "ASRT: %s\n      %s line %d function %s", (msg), __FILE__, __LINE__, __FUNCTION__);                                          \
        DEBUG_BREAK();                                                                                                                               \
    } while (0)
