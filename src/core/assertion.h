#pragma once

#if ENABLE_ASSERTS

#include "core/logger.h"

#if defined(_MSC_VER)
#define IMPL_DEBUG_BREAK __debugbreak
#elif defined(__GNUC__) || defined(__clang__)
#define IMPL_DEBUG_BREAK __builtin_trap
#else
// Unable to detect compiler, use C standard variant.
#include <cstdlib>
#define IMPL_DEBUG_BREAK abort
#endif

/* This function is not meant to be used outside of this file! */
namespace AssertImpl__ {
inline void fail_assert__(const char *expr, const char *file, int line, const char *message) {
    message = message ? message
                      : "No message specified";

    LOG_CRITICAL("ASSERTION FAILED\n"
                 "    Expression: {0}\n"
                 "    File: {1}:{2}\n"
                 "    Message: \"{3}\"",
        expr, file, line, message);

    IMPL_DEBUG_BREAK();
}
} // namespace AssertImpl__

#define IMPL_ASSERT_MSG(expr, msg)                                       \
    do {                                                                 \
        if (!((expr))) {                                                 \
            AssertImpl__::fail_assert__(#expr, __FILE__, __LINE__, msg); \
        }                                                                \
    } while (0);

#define IMPL_ASSERT_NO_MSG(expr)                                             \
    do {                                                                     \
        if (!((expr))) {                                                     \
            AssertImpl__::fail_assert__(#expr, __FILE__, __LINE__, nullptr); \
        }                                                                    \
    } while (0);

#define IMPL_ASSERT_GET(var1, var2, impl, ...) impl
#define IMPL_ASSERT_EXPAND(x) x

#define ASSERT(...) \
    IMPL_ASSERT_EXPAND(IMPL_ASSERT_GET(__VA_ARGS__, IMPL_ASSERT_MSG, IMPL_ASSERT_NO_MSG, 0)(__VA_ARGS__))

#else

/* do nothing and mark the expression as used */
#define ASSERT(expr, ...)   \
    do {                    \
        (void)sizeof(expr); \
    } while (0);

#endif // ENABLE_ASSERTS
