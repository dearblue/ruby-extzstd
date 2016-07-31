#include "extzstd.h"
#include "extzstd_nogvls.h"

/*
 * class Zstd::BufferedEncoder
 */

static void
bufenc_free(void *p)
{
    ZBUFF_freeCCtx(p);
}

AUX_IMPLEMENT_CONTEXT(
        ZBUFF_CCtx, bufenc_datatype, "extzstd.BufferedEncoder",
        bufenc_alloc_dummy, NULL, bufenc_free, NULL,
        getbufencp, getbufenc, bufenc_p);

static VALUE
bufenc_alloc(VALUE mod)
{
    ZBUFF_CCtx *p = ZBUFF_createCCtx();
    if (!p) {
        rb_gc();
        p = ZBUFF_createCCtx();
        if (!p) {
            rb_raise(extzstd_eError, "failed ZBUFF_createCCtx()");
        }
    }

    return TypedData_Wrap_Struct(mod, &bufenc_datatype, p);
}

static VALUE
bufenc_init(VALUE enc, VALUE params, VALUE dict)
{
    ZBUFF_CCtx *p = getbufenc(enc);
    const void *dictp;
    size_t dictsize;
    if (NIL_P(dict)) {
        dictp = NULL;
        dictsize = 0;
    } else {
        rb_check_type(dict, RUBY_T_STRING);
        RSTRING_GETMEM(dict, dictp, dictsize);
    }
    if (extzstd_encparams_p(params)) {
        ZSTD_parameters *paramsp = extzstd_getencparams(params);
        size_t s = aux_ZBUFF_compressInit_advanced(p, dictp, dictsize, paramsp, 0);
        extzstd_check_error(s);
    } else {
        size_t s = aux_ZBUFF_compressInitDictionary(p, dictp, dictsize, aux_num2int(params, 1));
        extzstd_check_error(s);
    }

    return enc;
}

static VALUE
bufenc_continue(VALUE v, VALUE src, VALUE srcoff, VALUE dest, VALUE maxsize)
{
    ZBUFF_CCtx *p = getbufenc(v);
    rb_check_type(src, RUBY_T_STRING);
    char *srcp;
    size_t srcsize;
    RSTRING_GETMEM(src, srcp, srcsize);
    size_t srcoff1 = NUM2SIZET(srcoff);
    if ((srcsize > 0 && srcoff1 >= srcsize) || srcoff1 > srcsize) {
        rb_raise(rb_eArgError,
                "``srcoff'' is out of src size (%"PRIuSIZE" for 0...%"PRIuSIZE")",
                srcoff1, srcsize);
    }
    srcp += srcoff1;
    srcsize -= srcoff1;
    size_t destsize = NUM2SIZET(maxsize);
    rb_check_type(dest, RUBY_T_STRING);
    aux_str_modify_expand(dest, destsize);
    rb_obj_infect(v, src);
    rb_obj_infect(dest, v);
    size_t s = aux_ZBUFF_compressContinue(p, RSTRING_PTR(dest), &destsize, srcp, &srcsize);
    extzstd_check_error(s);
    rb_str_set_len(dest, destsize);
    if (srcsize > 0) {
        return SIZET2NUM(srcoff1 + srcsize);
    } else {
        return Qnil;
    }
}

static VALUE
bufenc_flush(VALUE v, VALUE dest, VALUE maxsize)
{
    ZBUFF_CCtx *p = getbufenc(v);
    size_t destsize = NUM2SIZET(maxsize);
    rb_check_type(dest, RUBY_T_STRING);
    aux_str_modify_expand(dest, destsize);
    size_t s = aux_ZBUFF_compressFlush(p, RSTRING_PTR(dest), &destsize);
    extzstd_check_error(s);
    rb_str_set_len(dest, destsize);
    return dest;
}

static VALUE
bufenc_end(VALUE v, VALUE dest, VALUE maxsize)
{
    ZBUFF_CCtx *p = getbufenc(v);
    size_t destsize = NUM2SIZET(maxsize);
    rb_check_type(dest, RUBY_T_STRING);
    aux_str_modify_expand(dest, destsize);
    size_t s = aux_ZBUFF_compressEnd(p, RSTRING_PTR(dest), &destsize);
    extzstd_check_error(s);
    rb_str_set_len(dest, destsize);
    return dest;
}

static VALUE
bufenc_s_recommended_insize(VALUE mod)
{
    return SIZET2NUM(ZBUFF_recommendedCInSize());
}

static VALUE
bufenc_s_recommended_outsize(VALUE mod)
{
    return SIZET2NUM(ZBUFF_recommendedCOutSize());
}

static void
init_buffered_encoder(void)
{
    VALUE cBufEncoder = rb_define_class_under(extzstd_mZstd, "BufferedEncoder", rb_cObject);
    rb_define_alloc_func(cBufEncoder, bufenc_alloc);
    rb_define_method(cBufEncoder, "initialize", RUBY_METHOD_FUNC(bufenc_init), 2);
    rb_define_method(cBufEncoder, "continue", RUBY_METHOD_FUNC(bufenc_continue), 4);
    rb_define_method(cBufEncoder, "flush", RUBY_METHOD_FUNC(bufenc_flush), 2);
    rb_define_method(cBufEncoder, "end", RUBY_METHOD_FUNC(bufenc_end), 2);
    rb_define_singleton_method(cBufEncoder, "recommended_insize", RUBY_METHOD_FUNC(bufenc_s_recommended_insize), 0);
    rb_define_singleton_method(cBufEncoder, "recommended_outsize", RUBY_METHOD_FUNC(bufenc_s_recommended_outsize), 0);
}

/*
 * class Zstd::BufferedDecoder
 */

static void
bufdec_free(void *p)
{
    ZBUFF_freeDCtx(p);
}

AUX_IMPLEMENT_CONTEXT(
        ZBUFF_DCtx, bufdec_datatype, "extzstd.BufferedDecoder",
        bufdec_alloc_dummy, NULL, bufdec_free, NULL,
        getbufdecp, getbufdec, bufdec_p);

static VALUE
bufdec_alloc(VALUE mod)
{
    ZBUFF_DCtx *p = ZBUFF_createDCtx();
    if (!p) {
        rb_gc();
        p = ZBUFF_createDCtx();
        if (!p) {
            rb_raise(extzstd_eError, "failed ZBUFF_createDCtx()");
        }
    }

    return TypedData_Wrap_Struct(mod, &bufdec_datatype, p);
}

static VALUE
bufdec_init(VALUE dec, VALUE dict)
{
    /*
     * ZSTDLIB_API size_t ZBUFF_decompressInitDictionary(ZBUFF_DCtx* dctx, const void* dict, size_t dictSize);
     */

    ZBUFF_DCtx *p = getbufdec(dec);
    if (!NIL_P(dict)) {
        rb_check_type(dict, RUBY_T_STRING);
        if (RSTRING_LEN(dict) > 0) {
            dict = rb_str_new_frozen(dict);
            rb_obj_infect(dec, dict);
            size_t s = ZBUFF_decompressInitDictionary(p, RSTRING_PTR(dict), RSTRING_LEN(dict));
            extzstd_check_error(s);
            rb_iv_set(dec, "extzstd.dictionary for gc-guard", dict);
            return dec;
        }
    }

    size_t s = aux_ZBUFF_decompressInit(p);
    extzstd_check_error(s);

    return dec;
}

/*
 * call-seq:
 *  continue(src, srcoff, dest, maxdestsize) -> srcoff or nil
 */
static VALUE
bufdec_continue(VALUE dec, VALUE src, VALUE srcoff, VALUE dest, VALUE maxdest)
{
    ZBUFF_DCtx *p = getbufdec(dec);
    rb_check_type(src, RUBY_T_STRING);
    rb_check_type(dest, RUBY_T_STRING);
    const char *srcp;
    size_t srcsize;
    RSTRING_GETMEM(src, srcp, srcsize);
    size_t srcoff1 = NUM2SIZET(srcoff);
    if (srcoff1 >= srcsize) {
        rb_raise(rb_eArgError,
                "``srcoff'' is out of src size (%"PRIuSIZE" for 0...%"PRIuSIZE")",
                srcoff1, srcsize);
    }
    srcp += srcoff1;
    srcsize -= srcoff1;
    size_t destsize = NUM2SIZET(maxdest);
    aux_str_modify_expand(dest, destsize);
    char *destp = RSTRING_PTR(dest);
    rb_obj_infect(dec, src);
    rb_obj_infect(dest, dec);
    size_t s = aux_ZBUFF_decompressContinue(p, destp, &destsize, srcp, &srcsize);
    extzstd_check_error(s);
    rb_str_set_len(dest, destsize);
    if (srcsize == 0) {
        return Qnil;
    } else {
        return SIZET2NUM(srcoff1 + srcsize);
    }
}

static VALUE
bufdec_s_recommended_insize(VALUE mod)
{
    return SIZET2NUM(ZBUFF_recommendedDInSize());
}

static VALUE
bufdec_s_recommended_outsize(VALUE mod)
{
    return SIZET2NUM(ZBUFF_recommendedDOutSize());
}

static void
init_buffered_decoder(void)
{
    VALUE cBufDecoder = rb_define_class_under(extzstd_mZstd, "BufferedDecoder", rb_cObject);
    rb_define_alloc_func(cBufDecoder, bufdec_alloc);
    rb_define_method(cBufDecoder, "initialize", RUBY_METHOD_FUNC(bufdec_init), 1);
    rb_define_method(cBufDecoder, "continue", RUBY_METHOD_FUNC(bufdec_continue), 4);
    rb_define_singleton_method(cBufDecoder, "recommended_insize", RUBY_METHOD_FUNC(bufdec_s_recommended_insize), 0);
    rb_define_singleton_method(cBufDecoder, "recommended_outsize", RUBY_METHOD_FUNC(bufdec_s_recommended_outsize), 0);
}

/*
 * generic
 */

void
extzstd_init_buffered(void)
{
    init_buffered_encoder();
    init_buffered_decoder();
}
