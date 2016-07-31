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

static void *
aux_ZBUFF_compressInit_nogvl(va_list *vp)
{
    ZBUFF_CCtx *p = va_arg(*vp, ZBUFF_CCtx *);
    const void *dict = va_arg(*vp, const void *);
    size_t dictsize = va_arg(*vp, size_t);
    int level = va_arg(*vp, int);
    return (void *)ZBUFF_compressInitDictionary(p, dict, dictsize, level);
}

static inline size_t
aux_ZBUFF_compressInitDictionary(ZBUFF_CCtx *p, const void* dict, size_t dictsize, int level)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_compressInit_nogvl, NULL, p, dict, dictsize, level);
}

static inline void *
aux_ZBUFF_compressInit_advanced_nogvl(va_list *vp)
{
    ZBUFF_CCtx *p = va_arg(*vp, ZBUFF_CCtx *);
    const void *dict = va_arg(*vp, const void *);
    size_t dictsize = va_arg(*vp, size_t);
    const ZSTD_parameters *q = va_arg(*vp, const ZSTD_parameters *);
    unsigned long long pledgedsrcsize = va_arg(*vp, unsigned long long);
    return (void *)ZBUFF_compressInit_advanced(p, dict, dictsize, *q, pledgedsrcsize);
}

static inline size_t
aux_ZBUFF_compressInit_advanced(ZBUFF_CCtx *p, const void *dict, size_t dictsize, const ZSTD_parameters *q, unsigned long long pledgedsrcsize)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_compressInit_advanced_nogvl, NULL, p, dict, dictsize, q, pledgedsrcsize);
}

static inline void *
aux_ZBUFF_compressContinue_nogvl(va_list *vp)
{
    ZBUFF_CCtx *p = va_arg(*vp, ZBUFF_CCtx *);
    char *destp = va_arg(*vp, char *);
    size_t *destsize = va_arg(*vp, size_t *);
    const char *srcp = va_arg(*vp, const char *);
    size_t *srcsize = va_arg(*vp, size_t *);
    return (void *)ZBUFF_compressContinue(p, destp, destsize, srcp, srcsize);
}

static inline size_t
aux_ZBUFF_compressContinue(ZBUFF_CCtx *p, char *destp, size_t *destsize, const char *srcp, size_t *srcsize)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_compressContinue_nogvl, NULL, p, destp, destsize, srcp, srcsize);
}

static inline void *
aux_ZBUFF_compressFlush_nogvl(va_list *vp)
{
    ZBUFF_CCtx *p = va_arg(*vp, ZBUFF_CCtx *);
    char *destp = va_arg(*vp, char *);
    size_t *destsize = va_arg(*vp, size_t *);
    return (void *)ZBUFF_compressFlush(p, destp, destsize);
}

static inline size_t
aux_ZBUFF_compressFlush(ZBUFF_CCtx *p, char *destp, size_t *destsize)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_compressFlush_nogvl, NULL, p, destp, destsize);
}

static inline void *
aux_ZBUFF_compressEnd_nogvl(va_list *vp)
{
    ZBUFF_CCtx *p = va_arg(*vp, ZBUFF_CCtx *);
    char *destp = va_arg(*vp, char *);
    size_t *destsize = va_arg(*vp, size_t *);
    return (void *)ZBUFF_compressEnd(p, destp, destsize);
}

static inline size_t
aux_ZBUFF_compressEnd(ZBUFF_CCtx *p, char *destp, size_t *destsize)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_compressEnd_nogvl, NULL, p, destp, destsize);
}

static inline void *
aux_ZBUFF_decompressInit_nogvl(va_list *vp)
{
    ZBUFF_DCtx *p = va_arg(*vp, ZBUFF_DCtx *);
    return (void *)ZBUFF_decompressInit(p);
}

static inline size_t
aux_ZBUFF_decompressInit(ZBUFF_DCtx *p)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_decompressInit_nogvl, NULL, p);
}

static inline void *
aux_ZBUFF_decompressContinue_nogvl(va_list *vp)
{
    ZBUFF_DCtx *p = va_arg(*vp, ZBUFF_DCtx *);
    char *destp = va_arg(*vp, char *);
    size_t *destsize = va_arg(*vp, size_t *);
    const char *srcp = va_arg(*vp, const char *);
    size_t *srcsize = va_arg(*vp, size_t *);
    return (void *)ZBUFF_decompressContinue(p, destp, destsize, srcp, srcsize);
}

static inline size_t
aux_ZBUFF_decompressContinue(ZBUFF_DCtx *p, char *destp, size_t *destsize, const char *srcp, size_t *srcsize)
{
    return (size_t)aux_thread_call_without_gvl(
            aux_ZBUFF_decompressContinue_nogvl, NULL, p, destp, destsize, srcp, srcsize);
}

#endif /* EXTZSTD_NOGVLS_H */
