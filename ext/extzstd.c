#include "extzstd.h"

VALUE mZstd;
VALUE eError;

static size_t
aux_ZSTD_compress_nogvl(va_list *vp)
{
    char *dest = va_arg(*vp, char *);
    size_t destsize = va_arg(*vp, size_t);
    const char *src = va_arg(*vp, const char *);
    size_t srcsize = va_arg(*vp, size_t);
    return ZSTD_compress(dest, destsize, src, srcsize);
}

static inline size_t
aux_ZSTD_compress(char *dest, size_t destsize, const char *src, size_t srcsize)
{
    return (size_t)aux_thread_call_without_gvl(
            (void *(*)(void *))aux_ZSTD_compress_nogvl, NULL,
            dest, destsize, src, srcsize);
}

static inline void
zstd_s_encode_args(int argc, VALUE argv[], VALUE *src, VALUE *dest, size_t *maxsize)
{
    switch (argc) {
    case 1:
        *src = argv[0];
        rb_check_type(*src, RUBY_T_STRING);
        *maxsize = ZSTD_compressBound(RSTRING_LEN(*src));
        *dest = rb_str_buf_new(*maxsize);
        return;
    case 2:
        *src = argv[0];
        rb_check_type(*src, RUBY_T_STRING);
        *dest = argv[1];
        if (rb_type_p(*dest, RUBY_T_STRING)) {
            *maxsize = ZSTD_compressBound(RSTRING_LEN(*src));
            rb_str_resize(*dest, *maxsize);
        } else {
            *maxsize = NUM2SIZET(*dest);
            *dest = rb_str_buf_new(*maxsize);
        }
        return;
    case 3:
        *src = argv[0];
        rb_check_type(*src, RUBY_T_STRING);
        *maxsize = NUM2SIZET(argv[1]);
        *dest = argv[2];
        rb_check_type(*dest, RUBY_T_STRING);
        rb_str_resize(*dest, *maxsize);
        return;
    default:
        rb_error_arity(argc, 1, 3);
    }
}

/*
 * call-seq:
 *  encode(src) -> encoded string
 *  encode(src, size) -> encoded string
 *  encode(src, dest) -> dest with encoded string
 *  encode(src, size, dest) -> dest with encoded string
 */
static VALUE
zstd_s_encode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, dest;
    size_t maxsize;
    zstd_s_encode_args(argc, argv, &src, &dest, &maxsize);
    const char *srcp;
    size_t srcsize;
    RSTRING_GETMEM(src, srcp, srcsize);
    rb_obj_infect(dest, src);
    size_t s = aux_ZSTD_compress(RSTRING_PTR(dest), maxsize, srcp, srcsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                "failed ZSTD_compress - %s(%d) in %s:%d:%s",
                ZSTD_getErrorName(s), (int)s,
                __FILE__, __LINE__, __func__);
    }
    if (s > maxsize) {
        rb_bug("%s:%d:%s: detect buffer overflow in ZSTD_compress - maxsize is %zd, but returned size is %zd",
                __FILE__, __LINE__, __func__, maxsize, s);
    }
    rb_str_set_len(dest, s);
    return dest;
}

static size_t
aux_ZSTD_decompress_nogvl(va_list *vp)
{
    char *dest = va_arg(*vp, char *);
    size_t destsize = va_arg(*vp, size_t);
    const char *src = va_arg(*vp, const char *);
    size_t srcsize = va_arg(*vp, size_t);
    return ZSTD_decompress(dest, destsize, src, srcsize);
}

static inline size_t
aux_ZSTD_decompress(char *dest, size_t destsize, const char *src, size_t srcsize)
{
    return (size_t)aux_thread_call_without_gvl(
            (void *(*)(void *))aux_ZSTD_decompress_nogvl, NULL,
            dest, destsize, src, srcsize);
}

static inline void
zstd_s_decode_args(int argc, VALUE argv[], VALUE *src, VALUE *dest, size_t *maxsize)
{
    switch (argc) {
    case 2:
        *src = argv[0];
        rb_check_type(*src, RUBY_T_STRING);
        *maxsize = NUM2SIZET(argv[1]);
        *dest = rb_str_buf_new(*maxsize);
        return;
    case 3:
        *src = argv[0];
        rb_check_type(*src, RUBY_T_STRING);
        *maxsize = NUM2SIZET(argv[1]);
        *dest = argv[2];
        rb_check_type(*dest, RUBY_T_STRING);
        rb_str_resize(*dest, *maxsize);
        return;
    default:
        rb_error_arity(argc, 2, 3);
    }
}

/*
 * call-seq:
 *  decode(src, size) -> decoded string
 *  decode(src, size, dest) -> dest with decoded string
 */
static VALUE
zstd_s_decode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, dest;
    size_t maxsize;
    zstd_s_decode_args(argc, argv, &src, &dest, &maxsize);
    const char *srcp;
    size_t srcsize;
    RSTRING_GETMEM(src, srcp, srcsize);
    rb_obj_infect(dest, src);
    size_t s = aux_ZSTD_decompress(RSTRING_PTR(dest), maxsize, srcp, srcsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                "failed ZSTD_decompress - %s(%d) in %s:%d:%s",
                ZSTD_getErrorName(s), (int)s,
                __FILE__, __LINE__, __func__);
    }
    if (s > maxsize) {
        rb_bug("%s:%d:%s: detect buffer overflow in ZSTD_compress - maxsize is %zd, but returned size is %zd",
                __FILE__, __LINE__, __func__, maxsize, s);
    }
    rb_str_set_len(dest, s);
    return dest;
}

static VALUE
libver_s_to_s(VALUE ver)
{
    static VALUE str;
    if (!str) {
        str = rb_sprintf("%d.%d.%d",
                ZSTD_VERSION_MAJOR,
                ZSTD_VERSION_MINOR,
                ZSTD_VERSION_RELEASE);
    }
    return str;
}

void
Init_extzstd(void)
{
    mZstd = rb_define_module("Zstd");
    rb_define_singleton_method(mZstd, "encode", RUBY_METHOD_FUNC(zstd_s_encode), -1);
    rb_define_singleton_method(mZstd, "decode", RUBY_METHOD_FUNC(zstd_s_decode), -1);

    VALUE libver = rb_ary_new3(3,
            INT2FIX(ZSTD_VERSION_MAJOR),
            INT2FIX(ZSTD_VERSION_MINOR),
            INT2FIX(ZSTD_VERSION_RELEASE));
    rb_define_singleton_method(libver, "to_s", RUBY_METHOD_FUNC(libver_s_to_s), 0);
    rb_obj_freeze(libver);
    rb_define_const(mZstd, "LIBRARY_VERSION", libver);

    eError = rb_define_class_under(mZstd, "Error", rb_eRuntimeError);

    init_extzstd_stream();
}
