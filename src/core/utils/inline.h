/*
 * FORCE_INLINE C++ macro
 *
 * Forces the compiler to inline a function.
 * Returns an error if it can't be done (e.g. recursive functions)
 *
 * Taken from: https://meghprkh.github.io/blog/posts/c++-force-inline/
 * Check it out!
 */
#pragma once

/* Clang */
#if defined(__clang__)
#define FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline

/* GCC */
#elif defined(__GNUC__)
#define FORCE_INLINE [[gnu::always_inline]] inline

/* MSVC */
#elif define(_MSC_VER)
#pragma warning(error : 4714)
#define FORCE_INLINE __forceinline

#endif

#if !defined(FORCE_INLINE)
/* unsupported compiler, fallback to
 * standard inline keyword */
#define FORCE_INLINE inline
#endif
