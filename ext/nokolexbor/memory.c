/*
 * This file overrides the default `lexbor_malloc` and other
 * related functions to reduce memory bloat and allow Ruby to
 * GC more often.
 * By calling `ruby_xmalloc` instead of `malloc`, Ruby can
 * count the memory usage of the C extension and run GC
 * whenever `malloc_increase_byte` exceeds the limit. Similar
 * to Nokogiri's solution by calling `xmlMemSetup`.
 * The downside will be the downgrade of performance because
 * of more frequent GC.
 */

#include <ruby.h>
#include "lexbor/core/base.h"

// Disable using ruby memory functions when ASAN is enabled,
// otherwise memory leak info will be all about ruby which
// is useless.
#ifndef NOKOLEXBOR_ASAN

void *
lexbor_malloc(size_t size)
{
    return ruby_xmalloc(size);
}

void *
lexbor_realloc(void *dst, size_t size)
{
    return ruby_xrealloc(dst, size);
}

void *
lexbor_calloc(size_t num, size_t size)
{
    return ruby_xcalloc(num, size);
}

void *
lexbor_free(void *dst)
{
    ruby_xfree(dst);
    return NULL;
}

#endif