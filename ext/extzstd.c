#include "extzstd.h"

VALUE mZstd;
VALUE eError;

static inline void
zstd_encode_args(int argc, VALUE argv[], VALUE *src, VALUE *dest, size_t *maxsize)
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
zstd_encode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, dest;
    size_t maxsize;
    zstd_encode_args(argc, argv, &src, &dest, &maxsize);
    const char *srcp;
    size_t srcsize;
    RSTRING_GETMEM(src, srcp, srcsize);
    size_t s = ZSTD_compress(RSTRING_PTR(dest), maxsize, srcp, srcsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                 "%s:%d:%s: ZSTD_compress error - %s (%d)",
                 __FILE__, __LINE__, __func__,
                 ZSTD_getErrorName(s), (int)s);
    }
    if (s > maxsize) {
        rb_bug("%s:%d:%s: detect buffer overflow in ZSTD_compress - maxsize is %zd, but returned size is %zd",
                 __FILE__, __LINE__, __func__, maxsize, s);
    }
    rb_str_set_len(dest, s);
    return dest;
}

static inline void
zstd_decode_args(int argc, VALUE argv[], VALUE *src, VALUE *dest, size_t *maxsize)
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
zstd_decode(int argc, VALUE argv[], VALUE mod)
{
    VALUE src, dest;
    size_t maxsize;
    zstd_decode_args(argc, argv, &src, &dest, &maxsize);
    const char *srcp;
    size_t srcsize;
    RSTRING_GETMEM(src, srcp, srcsize);
    size_t s = ZSTD_decompress(RSTRING_PTR(dest), maxsize, srcp, srcsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                 "%s:%d:%s: ZSTD_compress error - %s (%d)",
                 __FILE__, __LINE__, __func__,
                 ZSTD_getErrorName(s), (int)s);
    }
    if (s > maxsize) {
        rb_bug("%s:%d:%s: detect buffer overflow in ZSTD_compress - maxsize is %zd, but returned size is %zd",
                 __FILE__, __LINE__, __func__, maxsize, s);
    }
    rb_str_set_len(dest, s);
    return dest;
}

void
Init_extzstd(void)
{
    mZstd = rb_define_module("Zstd");
    rb_define_singleton_method(mZstd, "encode", RUBY_METHOD_FUNC(zstd_encode), -1);
    rb_define_singleton_method(mZstd, "decode", RUBY_METHOD_FUNC(zstd_decode), -1);

    eError = rb_define_class_under(mZstd, "Error", rb_eRuntimeError);

    init_extzstd_stream();
}
