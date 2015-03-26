#include "extzstd.h"

VALUE cEncoder;
ID id_op_lshift;

struct zstdencoder
{
    VALUE outport;
    VALUE destbuf;
    ZSTD_cctx_t encoder;
};

static void
zstdenc_mark(void *pp)
{
    if (pp) {
        struct zstdencoder *p = pp;
        rb_gc_mark(p->outport);
        rb_gc_mark(p->destbuf);
    }
}

static void
zstdenc_free(void *pp)
{
    if (pp) {
        struct zstdencoder *p = pp;
        if (p->encoder) {
            ZSTD_freeCCtx(p->encoder);
        }
        if (!NIL_P(p->destbuf)) {
            rb_str_resize(p->destbuf, 0);
        }
        free(p);
    }
}

static const rb_data_type_t zstdencoder_type = {
    .wrap_struct_name = "extzstd.Zstd.Encoder",
    .function.dmark = zstdenc_mark,
    .function.dfree = zstdenc_free,
    /* .function.dsize = zstdenc_size, */
};


static VALUE
zstdenc_alloc(VALUE klass)
{
    struct zstdencoder *p;
    VALUE v = TypedData_Make_Struct(klass, struct zstdencoder, &zstdencoder_type, p);
    p->outport = Qnil;
    p->destbuf = Qnil;
    p->encoder = NULL;
    return v;
}

static inline struct zstdencoder *
getencoderp(VALUE enc)
{
    return getrefp(enc, &zstdencoder_type);
}

static inline struct zstdencoder *
getencoder(VALUE enc)
{
    return getref(enc, &zstdencoder_type);
}

static inline void
zstdenc_init_args(int argc, VALUE argv[], VALUE *outport)
{
    rb_scan_args(argc, argv, "01", outport);
    if (NIL_P(*outport)) {
        *outport = rb_str_buf_new(0);
    }
}

/*
 * call-seq:
 *  initialize(outport) -> self
 */
static VALUE
zstdenc_init(int argc, VALUE argv[], VALUE senc)
{
    struct zstdencoder *p = getencoder(senc);
    if (p->encoder) { reiniterror(senc); }
    zstdenc_init_args(argc, argv, &p->outport);
    p->destbuf = rb_str_buf_new(4 * 1024); /* FIXME: ``4 * 1024`` to constant value */
    p->encoder = ZSTD_createCCtx();
    if (!p->encoder) {
        rb_raise(eError,
                 "failed ZSTD_createCCtx()");
    }

    size_t destsize = rb_str_capacity(p->destbuf);
    //rb_str_resize(p->destbuf, destsize); /* FIXME: rb_str_resize は縮小もするので、できれば rb_str_modify_expand をつかう */

    size_t s = ZSTD_compressBegin(p->encoder, RSTRING_PTR(p->destbuf), destsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                 "failed ZSTD_compressBegin() - %s (%d)",
                 ZSTD_getErrorName(s), (int)s);
    }
    
    if (s > destsize) {
        rb_bug("detect buffer overflow from in ZSTD_compressBegin() (destbuf.bytesize=%d, returned bytesize=%d)",
               (int)RSTRING_LEN(p->destbuf), (int)s);
    }

    rb_str_set_len(p->destbuf, s);
    rb_funcall2(p->outport, id_op_lshift, 1, &p->destbuf);

    return senc;
}

/*
 * call-seq:
 *  write(src) -> self
 */
static VALUE
zstdenc_write(int argc, VALUE argv[], VALUE senc)
{
    struct zstdencoder *p = getencoder(senc);
    if (!p->encoder) { referror(senc); }

    VALUE src;
    rb_scan_args(argc, argv, "1", &src);
    rb_check_type(src, RUBY_T_STRING);

    size_t destsize = ZSTD_compressBound(RSTRING_LEN(src)) + 64; /* FIXME: おそらくストリームブロックヘッダのようなものがくっつく */
    rb_str_modify(p->destbuf);
    rb_str_set_len(p->destbuf, 0);
    rb_str_modify_expand(p->destbuf, destsize);

    size_t s = ZSTD_compressContinue(p->encoder, RSTRING_PTR(p->destbuf), destsize, RSTRING_PTR(src), RSTRING_LEN(src));
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                 "failed ZSTD_compressContinue() - %s (%d)",
                 ZSTD_getErrorName(s), (int)s);
    }
    
    if (s > destsize) {
        rb_bug("detect buffer overflow from in ZSTD_compressContinue() (destbuf.bytesize=%d, returned bytesize=%d)",
               (int)RSTRING_LEN(p->destbuf), (int)s);
    }

    rb_str_set_len(p->destbuf, s);
    rb_funcall2(p->outport, id_op_lshift, 1, &p->destbuf);

    return senc;
}

/*
 * call-seq:
 *  finish -> self
 */
static VALUE
zstdenc_finish(int argc, VALUE argv[], VALUE senc)
{
    struct zstdencoder *p = getencoder(senc);
    if (!p->encoder) { referror(senc); }

    rb_scan_args(argc, argv, "0");

    size_t destsize = 64; /* FIXME: ストリームフッターはどのくらい? */
    rb_str_modify(p->destbuf);
    rb_str_set_len(p->destbuf, 0);
    rb_str_modify_expand(p->destbuf, destsize);

    size_t s = ZSTD_compressEnd(p->encoder, RSTRING_PTR(p->destbuf), destsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                 "failed ZSTD_compressEnd() - %s (%d)",
                 ZSTD_getErrorName(s), (int)s);
    }

    if (s > destsize) {
        rb_bug("detect buffer overflow from in ZSTD_compressEnd() (destbuf.bytesize=%d, returned bytesize=%d)",
               (int)RSTRING_LEN(p->destbuf), (int)s);
    }

    rb_str_set_len(p->destbuf, s);
    rb_funcall2(p->outport, id_op_lshift, 1, &p->destbuf);

    return senc;
}

static VALUE
zstdenc_getoutport(VALUE enc)
{
    return getencoder(enc)->outport;
}

static VALUE
zstdenc_setoutport(VALUE enc, VALUE outport)
{
    return getencoder(enc)->outport = outport;
}

void
init_extzstd_stream(void)
{
    id_op_lshift = rb_intern("<<");

    cEncoder = rb_define_class_under(mZstd, "Encoder", rb_cObject);
    rb_define_alloc_func(cEncoder, zstdenc_alloc);
    rb_define_method(cEncoder, "initialize", RUBY_METHOD_FUNC(zstdenc_init), -1);
    rb_define_method(cEncoder, "write", RUBY_METHOD_FUNC(zstdenc_write), -1);
    rb_define_method(cEncoder, "finish", RUBY_METHOD_FUNC(zstdenc_finish), -1);
    rb_define_method(cEncoder, "outport", RUBY_METHOD_FUNC(zstdenc_getoutport), 0);
    rb_define_method(cEncoder, "outport=", RUBY_METHOD_FUNC(zstdenc_setoutport), 1);
/*
    cStreamDecoder = rb_define_class_under(mZstd, "StreamDecoder", rb_cObject);
    rb_define_alloc_func(cStreamDecoder, zstdenc_alloc);
    rb_define_method(cStreamDecoder, "initialize", RUBY_METHOD_FUNC(ext_sdec_init), -1);
    rb_define_method(cStreamDecoder, "read", RUBY_METHOD_FUNC(ext_sdec_read), -1);
    rb_define_method(cStreamDecoder, "finish", RUBY_METHOD_FUNC(ext_sdec_finish), -1);
*/
}
