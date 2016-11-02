#include "extzstd.h"
#include "extzstd_nogvls.h"
#include <errno.h>

/*
 * class Zstd::StreamEncoder
 */

static VALUE cStreamEncoder;

static void
enc_free(void *p)
{
    ZSTD_freeCStream(p);
}

AUX_IMPLEMENT_CONTEXT(
        ZSTD_CStream, encoder_type, "extzstd.Zstd::StreamEncoder",
        encoder_alloc_dummy, NULL, enc_free, NULL,
        getencoderp, getencoder, encoder_p);

static VALUE
enc_alloc(VALUE mod)
{
    VALUE obj = TypedData_Wrap_Struct(mod, &encoder_type, NULL);

    ZSTD_CStream *p = ZSTD_createCStream();
    if (!p) {
        rb_gc();
        p = ZSTD_createCStream();
        if (!p) {
            errno = ENOMEM;
            rb_sys_fail("failed ZSTD_createCStream()");
        }
    }
    DATA_PTR(obj) = p;

    return obj;
}

/*
 * call-seq:
 *  initialize(compression_parameters, predict)
 */
static VALUE
enc_init(VALUE self, VALUE params, VALUE predict)
{
    /*
     * ZSTDLIB_API size_t ZSTD_initCStream(ZSTD_CStream* zcs, int compressionLevel);
     * ZSTDLIB_API size_t ZSTD_initCStream_usingDict(ZSTD_CStream* zcs, const void* dict, size_t dictSize, int compressionLevel);
     * ZSTDLIB_API size_t ZSTD_initCStream_advanced(ZSTD_CStream* zcs, const void* dict, size_t dictSize,
     *                                              ZSTD_parameters params, unsigned long long pledgedSrcSize);
     */

    ZSTD_CStream *p = getencoder(self);
    const void *predictp;
    size_t predictsize;
    if (NIL_P(predict)) {
        predictp = NULL;
        predictsize = 0;
    } else {
        rb_check_type(predict, RUBY_T_STRING);
        RSTRING_GETMEM(predict, predictp, predictsize);
    }

    if (extzstd_params_p(params)) {
        ZSTD_parameters *paramsp = extzstd_getparams(params);
        size_t s = ZSTD_initCStream_advanced(p, predictp, predictsize, *paramsp, 0);
        extzstd_check_error(s);
    } else {
        size_t s = ZSTD_initCStream_usingDict(p, predictp, predictsize, aux_num2int(params, 1));
        extzstd_check_error(s);
    }
    return self;
}

static VALUE
enc_update(VALUE self, VALUE src, VALUE srcoff, VALUE dest, VALUE maxdest)
{
    /*
     * ZSTDLIB_API size_t ZSTD_compressStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output, ZSTD_inBuffer* input);
     */

    ZSTD_CStream *p = getencoder(self);

    rb_check_type(src, RUBY_T_STRING);
    const char *q = RSTRING_PTR(src);
    const char *qq = RSTRING_END(src);
    const char *q1 = q + NUM2SIZET(srcoff);
    if (q1 < q || q1 > qq) {
        rb_raise(rb_eArgError,
                "``srcoff'' is out of range (given %"PRIuSIZE", expect 0..%"PRIuSIZE")",
                q1 - q, qq - q);
    }

    rb_check_type(dest, RUBY_T_STRING);
    rb_str_modify(dest);
    rb_str_set_len(dest, 0);
    rb_str_modify_expand(dest, NUM2SIZET(maxdest));
    rb_obj_infect(self, src);
    rb_obj_infect(dest, self);
    char *r = RSTRING_PTR(dest);
    const char *rr = r + rb_str_capacity(dest);

    ZSTD_inBuffer input = { q, qq - q, q1 - q };
    ZSTD_outBuffer output = { r, rr - r, 0 };
    size_t s = ZSTD_compressStream(p, &output, &input);
    extzstd_check_error(s);
    rb_str_set_len(dest, output.pos);
    if (input.pos == input.size) {
        return Qnil;
    } else {
        return SIZET2NUM(input.pos);
    }
}

static VALUE
enc_flush(VALUE self, VALUE dest, VALUE maxdest)
{
    /*
     * ZSTDLIB_API size_t ZSTD_flushStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output);
     */

    ZSTD_CStream *p = getencoder(self);

    rb_check_type(dest, RUBY_T_STRING);
    rb_str_modify(dest);
    rb_str_set_len(dest, 0);
    rb_str_modify_expand(dest, NUM2SIZET(maxdest));

    ZSTD_outBuffer output = { RSTRING_PTR(dest), rb_str_capacity(dest), 0 };
    size_t s = ZSTD_flushStream(p, &output);
    extzstd_check_error(s);
    if (output.size > 0) {
        rb_str_set_len(dest, output.pos);
        return dest;
    } else {
        return Qnil;
    }
}

static VALUE
enc_end(VALUE self, VALUE dest, VALUE maxdest)
{
    /*
     * ZSTDLIB_API size_t ZSTD_endStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output);
     */

    ZSTD_CStream *p = getencoder(self);
    rb_check_type(dest, RUBY_T_STRING);
    rb_str_modify(dest);
    rb_str_set_len(dest, 0);
    rb_str_modify_expand(dest, NUM2SIZET(maxdest));

    ZSTD_outBuffer output = { RSTRING_PTR(dest), rb_str_capacity(dest), 0 };
    size_t s = ZSTD_endStream(p, &output);
    extzstd_check_error(s);
    if (output.size > 0) {
        rb_str_set_len(dest, output.pos);
        return dest;
    } else {
        return Qnil;
    }
}

static VALUE
enc_reset(VALUE self, VALUE pledged_srcsize)
{
    /*
     * ZSTDLIB_API size_t ZSTD_resetCStream(ZSTD_CStream* zcs, unsigned long long pledgedSrcSize);
     */

    size_t s = ZSTD_resetCStream(getencoder(self), NUM2ULL(pledged_srcsize));
    extzstd_check_error(s);
    return self;
}

static VALUE
enc_sizeof(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_sizeof_CStream(const ZSTD_CStream* zcs);
     */

    size_t s = ZSTD_sizeof_CStream(getencoder(self));
    extzstd_check_error(s);
    return SIZET2NUM(s);
}

static void
init_encoder(void)
{
    cStreamEncoder = rb_define_class_under(extzstd_mZstd, "StreamEncoder", rb_cObject);
    rb_define_alloc_func(cStreamEncoder, enc_alloc);
    rb_define_const(cStreamEncoder, "INSIZE", SIZET2NUM(ZSTD_CStreamInSize()));
    rb_define_const(cStreamEncoder, "OUTSIZE", SIZET2NUM(ZSTD_CStreamOutSize()));
    rb_define_method(cStreamEncoder, "initialize", enc_init, 2);
    rb_define_method(cStreamEncoder, "update", enc_update, 4);
    rb_define_method(cStreamEncoder, "flush", enc_flush, 2);
    rb_define_method(cStreamEncoder, "end", enc_end, 2);
    rb_define_method(cStreamEncoder, "reset", enc_reset, 1);
    rb_define_method(cStreamEncoder, "sizeof", enc_sizeof, 0);
}

/*
 * class Zstd::StreamDecoder
 */

static VALUE cStreamDecoder;

static void
dec_free(void *p)
{
    ZSTD_freeDStream(p);
}

AUX_IMPLEMENT_CONTEXT(
        ZSTD_DStream, decoder_type, "extzstd.Zstd::StreamDecoder",
        decoder_alloc_dummy, NULL, dec_free, NULL,
        getdecoderp, getdecoder, decoder_p);

static VALUE
dec_alloc(VALUE mod)
{
    VALUE obj = TypedData_Wrap_Struct(mod, &decoder_type, NULL);

    ZSTD_DStream *p = ZSTD_createDStream();
    if (!p) {
        rb_gc();
        p = ZSTD_createDStream();
        if (!p) {
            errno = ENOMEM;
            rb_sys_fail("failed ZSTD_createDStream()");
        }
    }
    DATA_PTR(obj) = p;

    return obj;
}

/*
 * call-seq:
 *  initialize(predict)
 */
static VALUE
dec_init(VALUE self, VALUE predict)
{
    /*
     * ZSTDLIB_API size_t ZSTD_initDStream(ZSTD_DStream* zds);
     * ZSTDLIB_API size_t ZSTD_initDStream_usingDict(ZSTD_DStream* zds, const void* dict, size_t dictSize);
     */

    ZSTD_DStream *p = getdecoder(self);

    if (NIL_P(predict)) {
        size_t s = ZSTD_initDStream(p);
        extzstd_check_error(s);
    } else {
        rb_check_type(predict, RUBY_T_STRING);
        size_t s = ZSTD_initDStream_usingDict(p, RSTRING_PTR(predict), RSTRING_LEN(predict));
        extzstd_check_error(s);
    }
    return self;
}

static VALUE
dec_update(VALUE self, VALUE src, VALUE srcoff, VALUE dest, VALUE maxdest)
{
    /*
     * ZSTDLIB_API size_t ZSTD_decompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output, ZSTD_inBuffer* input);
     */

    ZSTD_DStream *p = getdecoder(self);

    rb_check_type(src, RUBY_T_STRING);
    const char *q = RSTRING_PTR(src);
    const char *qq = RSTRING_END(src);
    const char *q1 = q + NUM2SIZET(srcoff);
    if (q1 < q || q1 > qq) {
        rb_raise(rb_eArgError,
                "``srcoff'' is out of range (given %"PRIuSIZE", expect 0..%"PRIuSIZE")",
                q1 - q, qq - q);
    }

    rb_check_type(dest, RUBY_T_STRING);
    rb_str_modify(dest);
    rb_str_set_len(dest, 0);
    rb_str_modify_expand(dest, NUM2SIZET(maxdest));
    rb_obj_infect(self, src);
    rb_obj_infect(dest, self);
    char *r = RSTRING_PTR(dest);
    const char *rr = r + rb_str_capacity(dest);

    ZSTD_inBuffer input = { q, qq - q, q1 - q };
    ZSTD_outBuffer output = { r, rr - r, 0 };
    size_t s = ZSTD_decompressStream(p, &output, &input);
    extzstd_check_error(s);
    rb_str_set_len(dest, output.pos);
    return SIZET2NUM(input.pos);
}

static VALUE
dec_reset(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_resetDStream(ZSTD_DStream* zds);
     */
    size_t s = ZSTD_resetDStream(getdecoder(self));
    extzstd_check_error(s);
    return self;
}

static VALUE
dec_sizeof(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_sizeof_DStream(const ZSTD_DStream* zds);
     */

    size_t s = ZSTD_sizeof_DStream(getdecoder(self));
    extzstd_check_error(s);
    return SIZET2NUM(s);
}

static void
init_decoder(void)
{
    cStreamDecoder = rb_define_class_under(extzstd_mZstd, "StreamDecoder", rb_cObject);
    rb_define_alloc_func(cStreamDecoder, dec_alloc);
    rb_define_const(cStreamDecoder, "INSIZE", SIZET2NUM(ZSTD_DStreamInSize()));
    rb_define_const(cStreamDecoder, "OUTSIZE", SIZET2NUM(ZSTD_DStreamOutSize()));
    rb_define_method(cStreamDecoder, "initialize", dec_init, 1);
    rb_define_method(cStreamDecoder, "update", dec_update, 4);
    rb_define_method(cStreamDecoder, "reset", dec_reset, 0);
    rb_define_method(cStreamDecoder, "sizeof", dec_sizeof, 0);
}

/*
 * initialize for extzstd_stream.c
 */

void
extzstd_init_stream(void)
{
    init_encoder();
    init_decoder();
}
