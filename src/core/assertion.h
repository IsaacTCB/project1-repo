#pragma once

// @TODO: use custom asssert solution over <cassert>
#include <cassert>

#if ENABLE_ASSERTS
#define IMPL_ASSERT_MSG(expr, msg) assert((expr) && msg)
#define IMPL_ASSERT_NO_MSG(expr) assert(expr)
#define IMPL_ASSERT_GET(var1, var2, impl, ...) impl
#define IMPL_ASSERT_EXPAND(x) x

#define ASSERT(...) \
    IMPL_ASSERT_EXPAND(IMPL_ASSERT_GET(__VA_ARGS__, IMPL_ASSERT_MSG, IMPL_ASSERT_NO_MSG, 0)(__VA_ARGS__))
#else
#define ASSERT(...) /* do nothing */
#endif
