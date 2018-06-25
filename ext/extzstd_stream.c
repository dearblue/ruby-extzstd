#include "extzstd.h"
#include "extzstd_nogvls.h"
#include <errno.h>

enum {
    EXT_PARTIAL_READ_SIZE = 256 * 1024, /* 256 KiB */
    EXT_READ_GROWUP_SIZE = 256 * 1024, /* 256 KiB */
    EXT_READ_DOUBLE_GROWUP_LIMIT_SIZE = 4 * 1024 * 1024, /* 4 MiB */
};

static inline VALUE
aux_str_buf_recycle(VALUE str, size_t capacity)
{
    if (!RTEST(str) || rb_obj_frozen_p(str) || !rb_type_p(str, RUBY_T_STRING)) {
        return rb_str_buf_new(capacity);
    } else {
        return aux_str_modify_expand(str, capacity);
    }
}

static ID id_op_lsh, id_read;

/*
 * class Zstd::Encoder
 */

static VALUE cStreamEncoder;

struct encoder
{
    ZSTD_CStream *context;
    VALUE outport;
    VALUE predict;
    VALUE destbuf;
    int reached_eof;
};

static void
enc_gc_mark(void *pp)
{
    if (pp) {
        struct encoder *p = (struct encoder *)pp;
        rb_gc_mark(p->outport);
        rb_gc_mark(p->predict);
        rb_gc_mark(p->destbuf);
    }
}

static void
enc_free(void *pp)
{
    if (pp) {
        struct encoder *p = (struct encoder *)pp;
        if (p->context) {
            ZSTD_freeCStream(p->context);
            p->context = NULL;
        }
        xfree(p);
    }
}

AUX_IMPLEMENT_CONTEXT(
        struct encoder, encoder_type, "extzstd.Zstd::Encoder",
        encoder_alloc_dummy, enc_gc_mark, enc_free, NULL,
        getencoderp, getencoder, encoder_p);

static VALUE
enc_alloc(VALUE mod)
{
    struct encoder *p;
    VALUE obj = TypedData_Make_Struct(mod, struct encoder, &encoder_type, p);
    p->outport = Qnil;
    p->predict = Qnil;
    p->destbuf = Qnil;
    return obj;
}

static struct encoder *
encoder_context(VALUE self)
{
    struct encoder *p = getencoder(self);
    if (!p->context) {
        rb_raise(rb_eTypeError,
                "wrong initialized context - #<%s:%p>",
                rb_obj_classname(self), (void *)self);
    }
    return p;
}

/*
 * call-seq:
 *  initialize(outport, compression_parameters = nil, predict = nil)
 */
static VALUE
enc_init(int argc, VALUE argv[], VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_initCStream(ZSTD_CStream* zcs, int compressionLevel);
     * ZSTDLIB_API size_t ZSTD_initCStream_usingDict(ZSTD_CStream* zcs, const void* dict, size_t dictSize, int compressionLevel);
     * ZSTDLIB_API size_t ZSTD_initCStream_advanced(ZSTD_CStream* zcs, const void* dict, size_t dictSize,
     *                                              ZSTD_parameters params, unsigned long long pledgedSrcSize);
     */

    VALUE outport, params, predict;
    switch (argc) {
    case 1:
        outport = argv[0];
        params = predict = Qnil;
        break;
    case 2:
        outport = argv[0];
        params = argv[1];
        predict = Qnil;
        break;
    case 3:
        outport = argv[0];
        params = argv[1];
        predict = argv[2];
        break;
    default:
        rb_error_arity(argc, 1, 3);
    }

    struct encoder *p = getencoder(self);
    if (p->context) {
        rb_raise(rb_eTypeError,
                "initialized already - #<%s:%p>",
                rb_obj_classname(self), (void *)self);
    }

    AUX_TRY_WITH_GC(
            p->context = ZSTD_createCStream(),
            "failed ZSTD_createCStream()");

    const void *predictp;
    size_t predictsize;
    if (NIL_P(predict)) {
        predictp = NULL;
        predictsize = 0;
    } else {
        rb_check_type(predict, RUBY_T_STRING);
        predict = rb_str_new_frozen(predict);
        RSTRING_GETMEM(predict, predictp, predictsize);
    }

    if (extzstd_params_p(params)) {
        ZSTD_parameters *paramsp = extzstd_getparams(params);
        size_t s = ZSTD_initCStream_advanced(p->context, predictp, predictsize, *paramsp, -1);
        extzstd_check_error(s);
    } else {
        size_t s = ZSTD_initCStream_usingDict(p->context, predictp, predictsize, aux_num2int(params, 1));
        extzstd_check_error(s);
    }

    p->predict = predict;
    p->outport = outport;

    return self;
}

static VALUE
enc_write(VALUE self, VALUE src)
{
    /*
     * ZSTDLIB_API size_t ZSTD_compressStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output, ZSTD_inBuffer* input);
     */

    struct encoder *p = encoder_context(self);
    src = rb_String(src);
    ZSTD_inBuffer input = { RSTRING_PTR(src), RSTRING_LEN(src), 0 };

    while (input.pos < input.size) {
        p->destbuf = aux_str_buf_recycle(p->destbuf, ZSTD_CStreamOutSize() * 2);
        rb_str_set_len(p->destbuf, 0);
        rb_obj_infect(self, src);
        rb_obj_infect(p->destbuf, self);
        ZSTD_outBuffer output = { RSTRING_PTR(p->destbuf), rb_str_capacity(p->destbuf), 0 };
        size_t s = ZSTD_compressStream(p->context, &output, &input);
        extzstd_check_error(s);
        rb_str_set_len(p->destbuf, output.pos);

        // TODO: 例外や帯域脱出した場合の挙動は?
        // TODO: src の途中経過状態を保存するべきか?
        AUX_FUNCALL(p->outport, id_op_lsh, p->destbuf);
    }

    return self;
}

static VALUE
enc_sync(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_flushStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output);
     */

    struct encoder *p = encoder_context(self);
    aux_str_buf_recycle(p->destbuf, ZSTD_CStreamOutSize());
    rb_str_set_len(p->destbuf, 0);
    rb_obj_infect(p->destbuf, self);
    ZSTD_outBuffer output = { RSTRING_PTR(p->destbuf), rb_str_capacity(p->destbuf), 0 };
    size_t s = ZSTD_flushStream(p->context, &output);
    extzstd_check_error(s);
    rb_str_set_len(p->destbuf, output.pos);

    AUX_FUNCALL(p->outport, id_op_lsh, p->destbuf);

    return self;
}

static VALUE
enc_close(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_endStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output);
     */

    struct encoder *p = encoder_context(self);
    aux_str_buf_recycle(p->destbuf, ZSTD_CStreamOutSize());
    rb_str_set_len(p->destbuf, 0);
    rb_obj_infect(p->destbuf, self);
    ZSTD_outBuffer output = { RSTRING_PTR(p->destbuf), rb_str_capacity(p->destbuf), 0 };
    size_t s = ZSTD_endStream(p->context, &output);
    extzstd_check_error(s);
    rb_str_set_len(p->destbuf, output.pos);

    AUX_FUNCALL(p->outport, id_op_lsh, p->destbuf);

    p->reached_eof = 1;

    return Qnil;
}

static VALUE
enc_eof(VALUE self)
{
    return (encoder_context(self)->reached_eof == 0 ? Qfalse : Qtrue);
}

static VALUE
enc_reset(VALUE self, VALUE pledged_srcsize)
{
    /*
     * ZSTDLIB_API size_t ZSTD_resetCStream(ZSTD_CStream* zcs, unsigned long long pledgedSrcSize);
     */

    size_t s = ZSTD_resetCStream(encoder_context(self)->context, NUM2ULL(pledged_srcsize));
    extzstd_check_error(s);
    return self;
}

static VALUE
enc_sizeof(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_sizeof_CStream(const ZSTD_CStream* zcs);
     */

    size_t s = ZSTD_sizeof_CStream(encoder_context(self)->context);
    extzstd_check_error(s);
    return SIZET2NUM(s);
}

static void
init_encoder(void)
{
    cStreamEncoder = rb_define_class_under(extzstd_mZstd, "Encoder", rb_cObject);
    rb_define_alloc_func(cStreamEncoder, enc_alloc);
    rb_define_const(cStreamEncoder, "INSIZE", SIZET2NUM(ZSTD_CStreamInSize()));
    rb_define_const(cStreamEncoder, "OUTSIZE", SIZET2NUM(ZSTD_CStreamOutSize()));
    rb_define_method(cStreamEncoder, "initialize", enc_init, -1);
    rb_define_method(cStreamEncoder, "write", enc_write, 1);
    rb_define_method(cStreamEncoder, "sync", enc_sync, 0);
    rb_define_method(cStreamEncoder, "close", enc_close, 0);
    rb_define_method(cStreamEncoder, "eof", enc_eof, 0);
    rb_define_alias(cStreamEncoder, "eof?", "eof");
    rb_define_method(cStreamEncoder, "reset", enc_reset, 1);
    rb_define_method(cStreamEncoder, "sizeof", enc_sizeof, 0);
    rb_define_alias(cStreamEncoder, "<<", "write");
    rb_define_alias(cStreamEncoder, "update", "write");
    rb_define_alias(cStreamEncoder, "flush", "sync");
    rb_define_alias(cStreamEncoder, "end", "close");
    rb_define_alias(cStreamEncoder, "finish", "close");
}

/*
 * class Zstd::Decoder
 */

static VALUE cStreamDecoder;

struct decoder
{
    ZSTD_DStream *context;
    VALUE inport;
    VALUE readbuf;
    VALUE predict;
    ZSTD_inBuffer inbuf;
    int reached_eof;
};

static void
dec_mark(void *pp)
{
    struct decoder *p = (struct decoder *)pp;
    rb_gc_mark(p->inport);
    rb_gc_mark(p->readbuf);
    rb_gc_mark(p->predict);
}

static void
dec_free(void *pp)
{
    struct decoder *p = (struct decoder *)pp;
    if (p->context) {
        ZSTD_freeDStream(p->context);
        p->context = NULL;
    }
    xfree(p);
}

AUX_IMPLEMENT_CONTEXT(
        struct decoder, decoder_type, "extzstd.Zstd::Decoder",
        decoder_alloc_dummy, dec_mark, dec_free, NULL,
        getdecoderp, getdecoder, decoder_p);

static struct decoder *
decoder_context(VALUE self)
{
    struct decoder *p = getdecoder(self);
    if (!p->context) {
        rb_raise(rb_eTypeError,
                "uninitialized context - #<%s:%p>",
                rb_obj_classname(self), (void *)self);
    }
    return p;
}

static VALUE
dec_alloc(VALUE mod)
{
    struct decoder *p;
    VALUE obj = TypedData_Make_Struct(mod, struct decoder, &decoder_type, p);
    return obj;
}

/*
 * call-seq:
 *  initialize(inport, predict = Qnil)
 */
static VALUE
dec_init(int argc, VALUE argv[], VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_initDStream(ZSTD_DStream* zds);
     * ZSTDLIB_API size_t ZSTD_initDStream_usingDict(ZSTD_DStream* zds, const void* dict, size_t dictSize);
     */

    VALUE inport, predict;

    switch (argc) {
    case 1:
        inport = argv[0];
        predict = Qnil;
        break;
    case 2:
        inport = argv[0];
        predict = argv[1];
        break;
    default:
        rb_error_arity(argc, 1, 2);
    }

    struct decoder *p = getdecoder(self);
    if (p->context) {
        rb_raise(rb_eTypeError,
                "initialized context already - #<%s:%p>",
                rb_obj_classname(self), (void *)self);
    }

    AUX_TRY_WITH_GC(
            p->context = ZSTD_createDStream(),
            "failed ZSTD_createDStream()");

    if (NIL_P(predict)) {
        size_t s = ZSTD_initDStream(p->context);
        extzstd_check_error(s);
    } else {
        rb_check_type(predict, RUBY_T_STRING);
        predict = rb_str_new_frozen(predict);
        size_t s = ZSTD_initDStream_usingDict(p->context, RSTRING_PTR(predict), RSTRING_LEN(predict));
        extzstd_check_error(s);
    }

    p->inport = inport;
    p->predict = predict;

    return self;
}

static int
dec_read_fetch(VALUE o, struct decoder *p)
{
    if (!p->inbuf.src || NIL_P(p->readbuf) || p->inbuf.pos >= RSTRING_LEN(p->readbuf)) {
        p->readbuf = aux_str_buf_recycle(p->readbuf, EXT_PARTIAL_READ_SIZE);
        VALUE st = AUX_FUNCALL(p->inport, id_read, INT2FIX(EXT_PARTIAL_READ_SIZE), p->readbuf);
        if (NIL_P(st)) { return -1; }
        rb_check_type(st, RUBY_T_STRING);
        p->readbuf = st;
        rb_obj_infect(o, p->readbuf);
        p->inbuf.size = RSTRING_LEN(p->readbuf);
        p->inbuf.pos = 0;
    }

    p->inbuf.src = RSTRING_PTR(p->readbuf);

    return 0;
}

static size_t
dec_read_decode(VALUE o, struct decoder *p, char *buf, ssize_t size)
{
    if (p->reached_eof != 0) {
        return 0;
    }

    ZSTD_outBuffer output = { buf, size, 0 };

    while (size < 0 || output.pos < size) {
        if (dec_read_fetch(o, p) != 0) {
            if (p->reached_eof == 0) {
                rb_raise(rb_eRuntimeError,
                         "unexpected EOF - #<%s:%p>",
                         rb_obj_classname(p->inport), (void *)p->inport);
            }

            break;
        }

        rb_thread_check_ints();
        size_t s = ZSTD_decompressStream(p->context, &output, &p->inbuf);
        extzstd_check_error(s);
        if (s == 0) {
            p->reached_eof = 1;
            break;
        }
    }

    return output.pos;
}

static void
dec_read_args(int argc, VALUE argv[], VALUE self, VALUE *buf, ssize_t *size)
{
    switch (argc) {
    case 0:
        *size = -1;
        *buf = rb_str_buf_new(EXT_READ_GROWUP_SIZE);
        break;
    case 1:
    case 2:
        {
            if (NIL_P(argv[0])) {
                *size = -1;

                if (argc == 1) {
                    *buf = rb_str_buf_new(EXT_READ_GROWUP_SIZE);
                } else {
                    rb_check_type(argv[1], RUBY_T_STRING);
                    *buf = aux_str_modify_expand(argv[1], EXT_READ_GROWUP_SIZE);
                    rb_str_set_len(*buf, 0);
                }
            } else {
                *size = NUM2SIZET(argv[0]);

                if (*size < 0) {
                    rb_raise(rb_eArgError,
                             "``size'' is negative or too large (%"PRIdPTR")",
                             (intptr_t)*size);
                }

                if (argc == 1) {
                    *buf = rb_str_buf_new(*size);
                } else {
                    rb_check_type(argv[1], RUBY_T_STRING);
                    *buf = aux_str_modify_expand(argv[1], *size);
                    rb_str_set_len(*buf, 0);
                }
            }
        }
        break;
    default:
        rb_error_arity(argc, 0, 2);
    }
}

/*
 * call-seq:
 *  read -> read_data
 *  read(readsize, buf = "".b) -> buf
 */
static VALUE
dec_read(int argc, VALUE argv[], VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_decompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output, ZSTD_inBuffer* input);
     */

    ssize_t size;
    VALUE buf;
    dec_read_args(argc, argv, self, &buf, &size);

    struct decoder *p = decoder_context(self);

    if (size == 0) {
        rb_str_set_len(buf, 0);
        return buf;
    } else if (size > 0) {
        size = dec_read_decode(self, p, RSTRING_PTR(buf), size);
        rb_str_set_len(buf, size);
    } else {
        /* if (size < 0) */

        size_t capa = EXT_READ_GROWUP_SIZE;

        for (;;) {
            aux_str_modify_expand(buf, capa);
            size = dec_read_decode(self, p, RSTRING_PTR(buf) + RSTRING_LEN(buf), capa - RSTRING_LEN(buf));
            rb_str_set_len(buf, RSTRING_LEN(buf) + size);
            if (size == 0) { break; }
            size = rb_str_capacity(buf);
            if (size > RSTRING_LEN(buf)) { break; }
            if (size > EXT_READ_DOUBLE_GROWUP_LIMIT_SIZE) {
                capa += EXT_READ_DOUBLE_GROWUP_LIMIT_SIZE;
            } else {
                capa *= 2;
            }
        }
    }

    rb_obj_infect(buf, self);

    if (RSTRING_LEN(buf) == 0) {
        return Qnil;
    } else {
        return buf;
    }
}

static VALUE
dec_eof(VALUE self)
{
    return (decoder_context(self)->reached_eof == 0 ? Qfalse : Qtrue);
}

static VALUE
dec_reset(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_resetDStream(ZSTD_DStream* zds);
     */
    size_t s = ZSTD_resetDStream(decoder_context(self)->context);
    extzstd_check_error(s);
    return self;
}

static VALUE
dec_sizeof(VALUE self)
{
    /*
     * ZSTDLIB_API size_t ZSTD_sizeof_DStream(const ZSTD_DStream* zds);
     */

    size_t s = ZSTD_sizeof_DStream(decoder_context(self)->context);
    extzstd_check_error(s);
    return SIZET2NUM(s);
}

static void
init_decoder(void)
{
    cStreamDecoder = rb_define_class_under(extzstd_mZstd, "Decoder", rb_cObject);
    rb_define_alloc_func(cStreamDecoder, dec_alloc);
    rb_define_const(cStreamDecoder, "INSIZE", SIZET2NUM(ZSTD_DStreamInSize()));
    rb_define_const(cStreamDecoder, "OUTSIZE", SIZET2NUM(ZSTD_DStreamOutSize()));
    rb_define_method(cStreamDecoder, "initialize", dec_init, -1);
    rb_define_method(cStreamDecoder, "read", dec_read, -1);
    rb_define_method(cStreamDecoder, "eof", dec_eof, 0);
    rb_define_alias(cStreamDecoder, "eof?", "eof");
    rb_define_method(cStreamDecoder, "reset", dec_reset, 0);
    rb_define_method(cStreamDecoder, "sizeof", dec_sizeof, 0);
}

/*
 * initialize for extzstd_stream.c
 */

void
extzstd_init_stream(void)
{
    id_op_lsh = rb_intern("<<");
    id_read = rb_intern("read");

    init_encoder();
    init_decoder();
}
