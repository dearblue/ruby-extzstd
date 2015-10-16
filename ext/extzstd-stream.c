#include "extzstd.h"

static ID id_op_lshift;

#if 0
static size_t
aux_read(VALUE io, size_t size, char *buf)
{
    VALUE w = SEND(io, id_read, SIZET2NUM(size));
    if (NIL_P(w)) { return 0; }
    rb_check_type(w, RUBY_T_STRING);
    size_t s = RSTRING_LEN(w);
    if (s > size) {
        rb_raise(rb_eRuntimeError,
                "read size is too big - #<%s:%p> (size is %"PRIuSIZE" for %"PRIuSIZE")",
                rb_obj_classname(io), (void *)io, s, size);
    }
    memcpy(buf, RSTRING_PTR(w), s);
    return s;
}
#endif

/*
 * class Zstd::Encoder
 */

static VALUE cEncoder;

struct encoder
{
    VALUE outport;
    VALUE destbuf;
    ZSTD_Cctx *encoder;
};

static void
enc_mark(void *pp)
{
    if (pp) {
        struct encoder *p = pp;
        rb_gc_mark(p->outport);
        rb_gc_mark(p->destbuf);
    }
}

static void
enc_free(void *pp)
{
    if (pp) {
        struct encoder *p = pp;
        if (p->encoder) {
            ZSTD_freeCCtx(p->encoder);
        }
        if (!NIL_P(p->destbuf)) {
            rb_str_resize(p->destbuf, 0);
        }
        free(p);
    }
}

static const rb_data_type_t encoder_type = {
    .wrap_struct_name = "extzstd.Zstd.Encoder",
    .function.dmark = enc_mark,
    .function.dfree = enc_free,
    /* .function.dsize = enc_size, */
};


static VALUE
enc_alloc(VALUE klass)
{
    struct encoder *p;
    VALUE v = TypedData_Make_Struct(klass, struct encoder, &encoder_type, p);
    p->outport = Qnil;
    p->destbuf = Qnil;
    p->encoder = NULL;
    return v;
}

static inline struct encoder *
getencoderp(VALUE enc)
{
    return getrefp(enc, &encoder_type);
}

static inline struct encoder *
getencoder(VALUE enc)
{
    return getref(enc, &encoder_type);
}

static inline void
enc_init_args(int argc, VALUE argv[], VALUE *outport)
{
    rb_scan_args(argc, argv, "01", outport);
    if (NIL_P(*outport)) {
        *outport = rb_str_buf_new(0);
    }
}

/*
 * call-seq:
 *  initialize(outport) -> self
 *
 * +outport+ is required +.<<+ method for behavior as same as +IO#<<+ method.
 */
static VALUE
enc_init(int argc, VALUE argv[], VALUE enc)
{
    struct encoder *p = getencoder(enc);
    if (p->encoder) { reiniterror(enc); }
    enc_init_args(argc, argv, &p->outport);
    p->destbuf = rb_str_buf_new(4 * 1024); /* FIXME: ``4 * 1024`` to constant value */
    p->encoder = ZSTD_createCCtx();
    if (!p->encoder) {
        rb_raise(eError,
                "failed ZSTD_createCCtx()");
    }

    size_t destsize = rb_str_capacity(p->destbuf);

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

    return enc;
}

/*
 * call-seq:
 *  write(src) -> self
 */
static VALUE
enc_write(int argc, VALUE argv[], VALUE enc)
{
    struct encoder *p = getencoder(enc);
    if (!p->encoder) { referror(enc); }

    VALUE src;
    rb_scan_args(argc, argv, "1", &src);
    rb_check_type(src, RUBY_T_STRING);

    size_t destsize = ZSTD_compressBound(RSTRING_LEN(src)) + 64; /* FIXME: おそらくストリームブロックヘッダのようなものがくっつく */
    rb_str_modify(p->destbuf);
    rb_str_set_len(p->destbuf, 0);
    rb_str_modify_expand(p->destbuf, destsize);
    rb_obj_infect(enc, src);
    rb_obj_infect(p->destbuf, enc);
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

    return enc;
}

/*
 * call-seq:
 *  finish -> self
 */
static VALUE
enc_finish(int argc, VALUE argv[], VALUE enc)
{
    struct encoder *p = getencoder(enc);
    if (!p->encoder) { referror(enc); }

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

    return enc;
}

static VALUE
enc_getoutport(VALUE enc)
{
    return getencoder(enc)->outport;
}

static VALUE
enc_setoutport(VALUE enc, VALUE outport)
{
    return getencoder(enc)->outport = outport;
}

/*
 * class Zstd::LowLevelDecoder
 */

static VALUE cLLDecoder;

struct lldecoder
{
    ZSTD_Dctx *decoder;
};

static void
lldec_mark(void *pp)
{
    if (pp) {
        struct lldecoder *p = pp;
    }
}

static void
lldec_free(void *pp)
{
    if (pp) {
        struct lldecoder *p = pp;
        if (p->decoder) {
            ZSTD_freeDCtx(p->decoder);
        }
        free(p);
    }
}

static const rb_data_type_t lldecoder_type = {
    .wrap_struct_name = "extzstd.Zstd.LowLevelDecoder",
    .function.dmark = lldec_mark,
    .function.dfree = lldec_free,
    /* .function.dsize = lldec_size, */
};


static VALUE
lldec_alloc(VALUE klass)
{
    struct lldecoder *p;
    VALUE v = TypedData_Make_Struct(klass, struct lldecoder, &lldecoder_type, p);
    p->decoder = NULL;
    return v;
}

static inline struct lldecoder *
getlldecoderp(VALUE v)
{
    return getrefp(v, &lldecoder_type);
}

static inline struct lldecoder *
getlldecoder(VALUE v)
{
    return getref(v, &lldecoder_type);
}

/*
 * call-seq:
 *  initialize
 */
static VALUE
lldec_init(int argc, VALUE argv[], VALUE dec)
{
    struct lldecoder *p = getlldecoder(dec);
    if (p->decoder) { reiniterror(dec); }
    rb_check_arity(argc, 0, 0);
    p->decoder = ZSTD_createDCtx();
    if (!p->decoder) {
        rb_raise(eError,
                "failed ZSTD_createDCtx()");
    }

    ZSTD_resetDCtx(p->decoder);

    return dec;
}

/*
 * call-seq:
 *  reset -> self
 */
static VALUE
lldec_reset(VALUE dec)
{
    struct lldecoder *p = getlldecoder(dec);
    if (!p->decoder) { referror(dec); }
    size_t s = ZSTD_resetDCtx(p->decoder);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                "failed ZSTD_resetDCtx() - %s (%d)",
                ZSTD_getErrorName(s), (int)s);
    }
    return dec;
}

/*
 * call-seq:
 *  next_srcsize -> next size as integer
 */
static VALUE
lldec_next_srcsize(VALUE dec)
{
    struct lldecoder *p = getlldecoder(dec);
    if (!p->decoder) { referror(dec); }
    size_t s = ZSTD_nextSrcSizeToDecompress(p->decoder);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                "failed ZSTD_nextSrcSizeToDecompress() - %s (%d)",
                ZSTD_getErrorName(s), (int)s);
    }
    return SIZET2NUM(s);
}

/*
 * call-seq:
 *  decode(src, dest, maxdestsize) -> dest OR nil
 */
static VALUE
lldec_decode(VALUE dec, VALUE src, VALUE dest, VALUE maxdestsize)
{
    struct lldecoder *p = getlldecoder(dec);
    if (!p->decoder) { referror(dec); }

    rb_obj_infect(dec, src);
    rb_obj_infect(dest, dec);
    const char *srcp = StringValuePtr(src);
    size_t srcsize = RSTRING_LEN(src);
    size_t destsize = NUM2UINT(maxdestsize); /* サイズ制限のために NUM2UINT にしている */
    aux_str_modify_expand(dest, destsize);
    char *destp = RSTRING_PTR(dest);
    size_t s = ZSTD_decompressContinue(p->decoder, destp, destsize, srcp, srcsize);
    if (ZSTD_isError(s)) {
        rb_raise(eError,
                "failed ZSTD_decompressContinue() - %s (%d)",
                ZSTD_getErrorName(s), (int)s);
    }
    rb_str_set_len(dest, s);
    if (s == 0) {
        return Qnil;
    } else {
        return dest;
    }
}

/*
 * initializer for Zstd::Encoder and Zstd::LowLevelDecoder
 */

void
init_extzstd_stream(void)
{
    id_op_lshift = rb_intern("<<");

    RDOCFAKE(mZstd = rb_define_module("Zstd"));

    cEncoder = rb_define_class_under(mZstd, "Encoder", rb_cObject);
    rb_define_alloc_func(cEncoder, enc_alloc);
    rb_define_method(cEncoder, "initialize", RUBY_METHOD_FUNC(enc_init), -1);
    rb_define_method(cEncoder, "write", RUBY_METHOD_FUNC(enc_write), -1);
    rb_define_method(cEncoder, "finish", RUBY_METHOD_FUNC(enc_finish), -1);
    rb_define_method(cEncoder, "outport", RUBY_METHOD_FUNC(enc_getoutport), 0);
    rb_define_method(cEncoder, "outport=", RUBY_METHOD_FUNC(enc_setoutport), 1);

    cLLDecoder = rb_define_class_under(mZstd, "LowLevelDecoder", rb_cObject);
    rb_define_alloc_func(cLLDecoder, lldec_alloc);
    rb_define_method(cLLDecoder, "initialize", RUBY_METHOD_FUNC(lldec_init), -1);
    rb_define_method(cLLDecoder, "reset", RUBY_METHOD_FUNC(lldec_reset), 0);
    rb_define_method(cLLDecoder, "next_srcsize", RUBY_METHOD_FUNC(lldec_next_srcsize), 0);
    rb_define_method(cLLDecoder, "decode", RUBY_METHOD_FUNC(lldec_decode), 3);
}
