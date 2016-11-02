#ifndef EXTZSTD_NOGVLS_H
#define EXTZSTD_NOGVLS_H 1

#include "extzstd.h"

static inline void *
aux_thread_call_without_gvl(void *(*func)(va_list *), void (*cancel)(va_list *), ...)
{
    va_list va1, va2;
    va_start(va1, cancel);
    va_start(va2, cancel);
    void *p = rb_thread_call_without_gvl(
            (void *(*)(void *))func, &va1,
            (void (*)(void *))cancel, &va2);
    va_end(va2);
    va_end(va1);
    return p;
}

static void *
aux_ZSTD_compress_nogvl(va_list *vp)
{
    char *dest = va_arg(*vp, char *);
    size_t destsize = va_arg(*vp, size_t);
    const char *src = va_arg(*vp, const char *);
    size_t srcsize = va_arg(*vp, size_t);
    int level = va_arg(*vp, int);
    return (void *)ZSTD_compress(dest, destsize, src, srcsize, level);
}

static inline size_t
aux_ZSTD_compress(char *dest, size_t destsize, const char *src, size_t srcsize, int level)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZSTD_compress_nogvl, NULL,
            dest, destsize, src, srcsize, level);
}

static void *
aux_ZSTD_decompress_nogvl(va_list *vp)
{
    char *dest = va_arg(*vp, char *);
    size_t destsize = va_arg(*vp, size_t);
    const char *src = va_arg(*vp, const char *);
    size_t srcsize = va_arg(*vp, size_t);
    return (void *)ZSTD_decompress(dest, destsize, src, srcsize);
}

static inline size_t
aux_ZSTD_decompress(char *dest, size_t destsize, const char *src, size_t srcsize)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZSTD_decompress_nogvl, NULL,
            dest, destsize, src, srcsize);
}

#endif /* EXTZSTD_NOGVLS_H */
