#include "extzstd.h"
#include <zstd/common/mem.h>
#include <zstd_errors.h>
#include <zdict.h>

static void
aux_string_pointer(VALUE str, const char **ptr, size_t *size)
{
    rb_check_type(str, RUBY_T_STRING);
    RSTRING_GETMEM(str, *ptr, *size);
}

static void
aux_string_pointer_with_nil(VALUE str, const char **ptr, size_t *size)
{
    if (NIL_P(str)) {
        *ptr = NULL;
        *size = 0;
    } else {
        aux_string_pointer(str, ptr, size);
    }
}

static void
aux_string_expand_pointer(VALUE str, char **ptr, size_t size)
{
    rb_check_type(str, RUBY_T_STRING);
    rb_str_modify(str);
    rb_str_set_len(str, 0);
    rb_str_modify_expand(str, size);
    *ptr = RSTRING_PTR(str);
}

VALUE extzstd_mZstd;

/*
 * constant Zstd::LIBRARY_VERSION
 */

static VALUE
libver_s_to_i(VALUE ver)
{
    return UINT2NUM(ZSTD_VERSION_NUMBER);
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
        rb_obj_freeze(str);
    }
    return str;
}

static void
init_libver(void)
{
    VALUE libver = AUX_TUPLE(
            INT2FIX(ZSTD_VERSION_MAJOR),
            INT2FIX(ZSTD_VERSION_MINOR),
            INT2FIX(ZSTD_VERSION_RELEASE));
    rb_define_singleton_method(libver, "to_i", RUBY_METHOD_FUNC(libver_s_to_i), 0);
    rb_define_singleton_method(libver, "to_s", RUBY_METHOD_FUNC(libver_s_to_s), 0);
    rb_define_singleton_method(libver, "to_str", RUBY_METHOD_FUNC(libver_s_to_s), 0);
    rb_obj_freeze(libver);
    rb_define_const(extzstd_mZstd, "LIBRARY_VERSION", libver);
}

/*
 * error classes
 */

/*
 * Document-class: Zstd::Exceptions
 *
 * Catch extzstd errors for rescue statment.
 */
VALUE extzstd_mExceptions;

VALUE extzstd_eError;
VALUE extzstd_eError;
VALUE extzstd_eGenericError;
VALUE extzstd_ePrefixUnknownError;
VALUE extzstd_eFrameParameterUnsupportedError;
VALUE extzstd_eFrameParameterUnsupportedBy32bitsError;
VALUE extzstd_eCompressionParameterUnsupportedError;
VALUE extzstd_eInitMissingError;
VALUE extzstd_eMemoryAllocationError;
VALUE extzstd_eStageWrongError;
VALUE extzstd_eDstSizeTooSmallError;
VALUE extzstd_eSrcSizeWrongError;
VALUE extzstd_eCorruptionDetectedError;
VALUE extzstd_eChecksumWrongError;
VALUE extzstd_eTableLogTooLargeError;
VALUE extzstd_eMaxSymbolValueTooLargeError;
VALUE extzstd_eMaxSymbolValueTooSmallError;
VALUE extzstd_eDictionaryCorruptedError;
VALUE extzstd_eDictionaryWrongError;


void
extzstd_check_error(ssize_t errcode)
{
    if (ZSTD_isError(errcode)) {
        extzstd_error(errcode);
    }
}

void
extzstd_error(ssize_t errcode)
{
    rb_exc_raise(extzstd_make_error(errcode));
}

VALUE
extzstd_make_errorf(VALUE exc, const char *fmt, ...)
{
    if (fmt && strlen(fmt) > 0) {
        va_list va;
        va_start(va, fmt);
        VALUE mesg = rb_vsprintf(fmt, va);
        va_end(va);
        return rb_exc_new3(exc, mesg);
    } else {
        return rb_exc_new2(exc, "");
    }
}

VALUE
extzstd_make_error(ssize_t errcode)
{
    if (!ZSTD_isError(errcode)) { return Qnil; }

#define CASE_ERROR(zstderr, extzstderr, ...)                            \
    case zstderr: return extzstd_make_errorf(extzstderr, __VA_ARGS__);  \

    switch (-errcode) {
    CASE_ERROR(ZSTD_error_GENERIC, extzstd_eGenericError, NULL);
    CASE_ERROR(ZSTD_error_prefix_unknown, extzstd_ePrefixUnknownError, NULL);
    CASE_ERROR(ZSTD_error_frameParameter_unsupported, extzstd_eFrameParameterUnsupportedError, NULL);
    CASE_ERROR(ZSTD_error_frameParameter_unsupportedBy32bits, extzstd_eFrameParameterUnsupportedBy32bitsError, NULL);
    CASE_ERROR(ZSTD_error_compressionParameter_unsupported, extzstd_eCompressionParameterUnsupportedError, NULL);
    CASE_ERROR(ZSTD_error_init_missing, extzstd_eInitMissingError, NULL);
    CASE_ERROR(ZSTD_error_memory_allocation, extzstd_eMemoryAllocationError, NULL);
    CASE_ERROR(ZSTD_error_stage_wrong, extzstd_eStageWrongError, NULL);
    CASE_ERROR(ZSTD_error_dstSize_tooSmall, extzstd_eDstSizeTooSmallError, NULL);
    CASE_ERROR(ZSTD_error_srcSize_wrong, extzstd_eSrcSizeWrongError, NULL);
    CASE_ERROR(ZSTD_error_corruption_detected, extzstd_eCorruptionDetectedError, NULL);
    CASE_ERROR(ZSTD_error_checksum_wrong, extzstd_eChecksumWrongError, NULL);
    CASE_ERROR(ZSTD_error_tableLog_tooLarge, extzstd_eTableLogTooLargeError, NULL);
    CASE_ERROR(ZSTD_error_maxSymbolValue_tooLarge, extzstd_eMaxSymbolValueTooLargeError, NULL);
    CASE_ERROR(ZSTD_error_maxSymbolValue_tooSmall, extzstd_eMaxSymbolValueTooSmallError, NULL);
    CASE_ERROR(ZSTD_error_dictionary_corrupted, extzstd_eDictionaryCorruptedError, NULL);
    CASE_ERROR(ZSTD_error_dictionary_wrong, extzstd_eDictionaryWrongError, NULL);
    default:
        return extzstd_make_errorf(extzstd_eError,
                "unknown zstd error code (%d)", errcode);
    }

#undef CASE_ERROR
}

static void
init_error(void)
{
    extzstd_mExceptions = rb_define_module_under(extzstd_mZstd, "Exceptions");

    extzstd_eError = rb_define_class_under(extzstd_mZstd, "Error", rb_eRuntimeError);
    rb_include_module(extzstd_eError, extzstd_mExceptions);

    extzstd_eGenericError = rb_define_class_under(extzstd_mZstd, "GenericError", rb_eRuntimeError);
    rb_include_module(extzstd_eGenericError, extzstd_mExceptions);

    extzstd_ePrefixUnknownError = rb_define_class_under(extzstd_mZstd, "PrefixUnknownError", rb_eArgError);
    rb_include_module(extzstd_ePrefixUnknownError, extzstd_mExceptions);

    extzstd_eFrameParameterUnsupportedError = rb_define_class_under(extzstd_mZstd, "FrameParameterUnsupportedError", rb_eRuntimeError);
    rb_include_module(extzstd_eFrameParameterUnsupportedError, extzstd_mExceptions);

    extzstd_eFrameParameterUnsupportedBy32bitsError = rb_define_class_under(extzstd_mZstd, "FrameParameterUnsupportedBy32bitsImplementationError", rb_eRuntimeError);
    rb_include_module(extzstd_eFrameParameterUnsupportedBy32bitsError, extzstd_mExceptions);

    extzstd_eCompressionParameterUnsupportedError = rb_define_class_under(extzstd_mZstd, "CompressionParameterUnsupportedError", rb_eRuntimeError);
    rb_include_module(extzstd_eCompressionParameterUnsupportedError, extzstd_mExceptions);

    extzstd_eInitMissingError = rb_define_class_under(extzstd_mZstd, "InitMissingError", rb_eRuntimeError);
    rb_include_module(extzstd_eInitMissingError, extzstd_mExceptions);

    extzstd_eMemoryAllocationError = rb_define_class_under(extzstd_mZstd, "MemoryAllocationError", aux_const_dig_str(rb_cObject, "Errno", "ENOMEM"));
    rb_include_module(extzstd_eMemoryAllocationError, extzstd_mExceptions);

    extzstd_eStageWrongError = rb_define_class_under(extzstd_mZstd, "StageWrongError", rb_eRuntimeError);
    rb_include_module(extzstd_eStageWrongError, extzstd_mExceptions);

    extzstd_eDstSizeTooSmallError = rb_define_class_under(extzstd_mZstd, "DstSizeTooSmallError", rb_eArgError);
    rb_include_module(extzstd_eDstSizeTooSmallError, extzstd_mExceptions);

    extzstd_eSrcSizeWrongError = rb_define_class_under(extzstd_mZstd, "SrcSizeWrongError", rb_eArgError);
    rb_include_module(extzstd_eSrcSizeWrongError, extzstd_mExceptions);

    extzstd_eCorruptionDetectedError = rb_define_class_under(extzstd_mZstd, "CorruptionDetectedError", rb_eRuntimeError);
    rb_include_module(extzstd_eCorruptionDetectedError, extzstd_mExceptions);

    extzstd_eChecksumWrongError = rb_define_class_under(extzstd_mZstd, "ChecksumWrongError", rb_eRuntimeError);
    rb_include_module(extzstd_eChecksumWrongError, extzstd_mExceptions);

    extzstd_eTableLogTooLargeError = rb_define_class_under(extzstd_mZstd, "TableLogTooLargeError", rb_eArgError);
    rb_include_module(extzstd_eTableLogTooLargeError, extzstd_mExceptions);

    extzstd_eMaxSymbolValueTooLargeError = rb_define_class_under(extzstd_mZstd, "MaxSymbolValueTooLargeError", rb_eArgError);
    rb_include_module(extzstd_eMaxSymbolValueTooLargeError, extzstd_mExceptions);

    extzstd_eMaxSymbolValueTooSmallError = rb_define_class_under(extzstd_mZstd, "MaxSymbolValueTooSmallError", rb_eArgError);
    rb_include_module(extzstd_eMaxSymbolValueTooSmallError, extzstd_mExceptions);

    extzstd_eDictionaryCorruptedError = rb_define_class_under(extzstd_mZstd, "DictionaryCorruptedError", rb_eRuntimeError);
    rb_include_module(extzstd_eDictionaryCorruptedError, extzstd_mExceptions);

    extzstd_eDictionaryWrongError = rb_define_class_under(extzstd_mZstd, "DictionaryWrongError", rb_eRuntimeError);
    rb_include_module(extzstd_eDictionaryWrongError, extzstd_mExceptions);
}

/*
 * module Zstd::Constants
 */

static void
init_constants(void)
{
    VALUE mConstants = rb_define_module_under(extzstd_mZstd, "Constants");
    rb_include_module(extzstd_mZstd, mConstants);

    rb_define_const(mConstants, "ZSTD_MAX_COMPRESSION_LEVEL", INT2NUM(ZSTD_maxCLevel()));
    rb_define_const(mConstants, "MAX_COMPRESSION_LEVEL", INT2NUM(ZSTD_maxCLevel()));

    rb_define_const(mConstants, "ZSTD_FAST", INT2NUM(ZSTD_fast));
    rb_define_const(mConstants, "ZSTD_DFAST", INT2NUM(ZSTD_dfast));
    rb_define_const(mConstants, "ZSTD_GREEDY", INT2NUM(ZSTD_greedy));
    rb_define_const(mConstants, "ZSTD_LAZY", INT2NUM(ZSTD_lazy));
    rb_define_const(mConstants, "ZSTD_LAZY2", INT2NUM(ZSTD_lazy2));
    rb_define_const(mConstants, "ZSTD_BTLAZY2", INT2NUM(ZSTD_btlazy2));
    rb_define_const(mConstants, "ZSTD_BTOPT", INT2NUM(ZSTD_btopt));
    rb_define_const(mConstants, "ZSTD_WINDOWLOG_MAX", INT2NUM(ZSTD_WINDOWLOG_MAX));
    rb_define_const(mConstants, "ZSTD_WINDOWLOG_MIN", INT2NUM(ZSTD_WINDOWLOG_MIN));
    rb_define_const(mConstants, "ZSTD_HASHLOG_MAX", INT2NUM(ZSTD_HASHLOG_MAX));
    rb_define_const(mConstants, "ZSTD_HASHLOG_MIN", INT2NUM(ZSTD_HASHLOG_MIN));
    rb_define_const(mConstants, "ZSTD_CHAINLOG_MAX", INT2NUM(ZSTD_CHAINLOG_MAX));
    rb_define_const(mConstants, "ZSTD_CHAINLOG_MIN", INT2NUM(ZSTD_CHAINLOG_MIN));
    rb_define_const(mConstants, "ZSTD_HASHLOG3_MAX", INT2NUM(ZSTD_HASHLOG3_MAX));
    rb_define_const(mConstants, "ZSTD_SEARCHLOG_MAX", INT2NUM(ZSTD_SEARCHLOG_MAX));
    rb_define_const(mConstants, "ZSTD_SEARCHLOG_MIN", INT2NUM(ZSTD_SEARCHLOG_MIN));
    rb_define_const(mConstants, "ZSTD_SEARCHLENGTH_MAX", INT2NUM(ZSTD_SEARCHLENGTH_MAX));
    rb_define_const(mConstants, "ZSTD_SEARCHLENGTH_MIN", INT2NUM(ZSTD_SEARCHLENGTH_MIN));
    rb_define_const(mConstants, "ZSTD_TARGETLENGTH_MAX", INT2NUM(ZSTD_TARGETLENGTH_MAX));
    rb_define_const(mConstants, "ZSTD_TARGETLENGTH_MIN", INT2NUM(ZSTD_TARGETLENGTH_MIN));

    rb_define_const(mConstants, "FAST", INT2NUM(ZSTD_fast));
    rb_define_const(mConstants, "DFAST", INT2NUM(ZSTD_dfast));
    rb_define_const(mConstants, "GREEDY", INT2NUM(ZSTD_greedy));
    rb_define_const(mConstants, "LAZY", INT2NUM(ZSTD_lazy));
    rb_define_const(mConstants, "LAZY2", INT2NUM(ZSTD_lazy2));
    rb_define_const(mConstants, "BTLAZY2", INT2NUM(ZSTD_btlazy2));
    rb_define_const(mConstants, "BTOPT", INT2NUM(ZSTD_btopt));
    rb_define_const(mConstants, "WINDOWLOG_MAX", INT2NUM(ZSTD_WINDOWLOG_MAX));
    rb_define_const(mConstants, "WINDOWLOG_MIN", INT2NUM(ZSTD_WINDOWLOG_MIN));
    rb_define_const(mConstants, "HASHLOG_MAX", INT2NUM(ZSTD_HASHLOG_MAX));
    rb_define_const(mConstants, "HASHLOG_MIN", INT2NUM(ZSTD_HASHLOG_MIN));
    rb_define_const(mConstants, "CHAINLOG_MAX", INT2NUM(ZSTD_CHAINLOG_MAX));
    rb_define_const(mConstants, "CHAINLOG_MIN", INT2NUM(ZSTD_CHAINLOG_MIN));
    rb_define_const(mConstants, "HASHLOG3_MAX", INT2NUM(ZSTD_HASHLOG3_MAX));
    rb_define_const(mConstants, "SEARCHLOG_MAX", INT2NUM(ZSTD_SEARCHLOG_MAX));
    rb_define_const(mConstants, "SEARCHLOG_MIN", INT2NUM(ZSTD_SEARCHLOG_MIN));
    rb_define_const(mConstants, "SEARCHLENGTH_MAX", INT2NUM(ZSTD_SEARCHLENGTH_MAX));
    rb_define_const(mConstants, "SEARCHLENGTH_MIN", INT2NUM(ZSTD_SEARCHLENGTH_MIN));
    rb_define_const(mConstants, "TARGETLENGTH_MAX", INT2NUM(ZSTD_TARGETLENGTH_MAX));
    rb_define_const(mConstants, "TARGETLENGTH_MIN", INT2NUM(ZSTD_TARGETLENGTH_MIN));
}

/*
 * class Zstd::EncodeParameter
 */

VALUE extzstd_cParams;

AUX_IMPLEMENT_CONTEXT(
        ZSTD_parameters, params_type, "extzstd.Parameters",
        params_alloc_dummy, NULL, free, NULL,
        getparamsp, getparams, params_p);

ZSTD_parameters *
extzstd_getparams(VALUE v)
{
    return getparams(v);
}

int
extzstd_params_p(VALUE v)
{
    return params_p(v);
}

static VALUE
params_alloc(VALUE mod)
{
    ZSTD_parameters *p;
    return TypedData_Make_Struct(mod, ZSTD_parameters, &params_type, p);
}

VALUE
extzstd_params_alloc(ZSTD_parameters **p)
{
    return TypedData_Make_Struct(extzstd_cParams, ZSTD_parameters, &params_type, *p);
}

/*
 * call-seq:
 *  initialize(preset_level = 1, srcsize_hint = 0, dictsize = 0, opts = {})
 *
 * Initialize struct ZSTD_parameters of C layer.
 *
 * [preset_level = 1]
 * [srcsize_hint = 0]
 * [opts windowlog: nil]
 * [opts contentlog: nil]
 * [opts hashlog: nil]
 * [opts searchlog: nil]
 * [opts searchlength: nil]
 * [opts targetlength: nil]
 * [opts strategy: nil]
 */
static VALUE
params_init(int argc, VALUE argv[], VALUE v)
{
    ZSTD_parameters *p = getparams(v);
    uint64_t sizehint;
    size_t dictsize;
    int level;
    VALUE opts = Qnil;

    argc = rb_scan_args(argc, argv, "03:", NULL, NULL, NULL, &opts);
    level = argc > 0 ? aux_num2int(argv[0], 0) : 0;
    sizehint = argc > 1 ? aux_num2int_u64(argv[1], 0) : 0;
    dictsize = argc > 2 ? aux_num2int_u64(argv[2], 0) : 0;

    *p = ZSTD_getParams(level, sizehint, dictsize);

    if (!NIL_P(opts)) {
#define SETUP_PARAM(var, opts, key, converter)                          \
        do {                                                            \
            VALUE tmp = rb_hash_lookup(opts, ID2SYM(rb_intern(key)));   \
            if (!NIL_P(tmp)) {                                          \
                var = converter(tmp);                                   \
            }                                                           \
        } while (0)                                                     \

        SETUP_PARAM(p->cParams.windowLog, opts, "windowlog", NUM2UINT);
        SETUP_PARAM(p->cParams.chainLog, opts, "chainlog", NUM2UINT);
        SETUP_PARAM(p->cParams.hashLog, opts, "hashlog", NUM2UINT);
        SETUP_PARAM(p->cParams.searchLog, opts, "searchlog", NUM2UINT);
        SETUP_PARAM(p->cParams.searchLength, opts, "searchlength", NUM2UINT);
        SETUP_PARAM(p->cParams.targetLength, opts, "targetlength", NUM2UINT);
        SETUP_PARAM(p->cParams.strategy, opts, "strategy", NUM2UINT);
#undef SETUP_PARAM
    }

    return v;
}

static VALUE
params_init_copy(VALUE params, VALUE src)
{
    ZSTD_parameters *a = getparams(params);
    ZSTD_parameters *b = getparams(src);
    rb_check_frozen(params);
    rb_obj_infect(params, src);
    memcpy(a, b, sizeof(*a));
    return params;
}

//static VALUE
//params_validate(VALUE v)
//{
//    ZSTD_validateParams(getparams(v));
//    return v;
//}

#define IMP_PARAMS(GETTER, SETTER, FIELD)                   \
    static VALUE                                            \
    GETTER(VALUE v)                                         \
    {                                                       \
        return UINT2NUM(getparams(v)->cParams.FIELD);    \
    }                                                       \
                                                            \
    static VALUE                                            \
    SETTER(VALUE v, VALUE n)                                \
    {                                                       \
        getparams(v)->cParams.FIELD = NUM2UINT(n);       \
        return n;                                           \
    }                                                       \

//IMP_PARAMS(params_srcsize, params_set_srcsize, srcSize);
IMP_PARAMS(params_windowlog, params_set_windowlog, windowLog);
IMP_PARAMS(params_chainlog, params_set_chainlog, chainLog);
IMP_PARAMS(params_hashlog, params_set_hashlog, hashLog);
IMP_PARAMS(params_searchlog, params_set_searchlog, searchLog);
IMP_PARAMS(params_searchlength, params_set_searchlength, searchLength);
IMP_PARAMS(params_targetlength, params_set_targetlength, targetLength);
IMP_PARAMS(params_strategy, params_set_strategy, strategy);

#undef IMP_PARAMS

static VALUE
params_s_get_preset(int argc, VALUE argv[], VALUE mod)
{
    int level;
    uint64_t sizehint;
    size_t dictsize;

    switch (argc) {
    case 0:
        level = 0;
        sizehint = 0;
        dictsize = 0;
        break;
    case 1:
        level = NUM2INT(argv[0]);
        sizehint = 0;
        dictsize = 0;
        break;
    case 2:
        level = NUM2INT(argv[0]);
        sizehint = NUM2ULL(argv[1]);
        dictsize = 0;
        break;
    case 3:
        level = NUM2INT(argv[0]);
        sizehint = NUM2ULL(argv[1]);
        dictsize = NUM2SIZET(argv[2]);
        break;
    default:
        rb_error_arity(argc, 0, 3);
    }

    ZSTD_parameters *p;
    VALUE v = TypedData_Make_Struct(mod, ZSTD_parameters, &params_type, p);
    *p = ZSTD_getParams(level, sizehint, dictsize);
    return v;
}

/*
 * Document-method: Zstd::Parameters#windowlog
 * Document-method: Zstd::Parameters#windowlog=
 * Document-method: Zstd::Parameters#chainlog
 * Document-method: Zstd::Parameters#chainlog=
 * Document-method: Zstd::Parameters#hashlog
 * Document-method: Zstd::Parameters#hashlog=
 * Document-method: Zstd::Parameters#searchlog
 * Document-method: Zstd::Parameters#searchlog=
 * Document-method: Zstd::Parameters#searchlength
 * Document-method: Zstd::Parameters#searchlength=
 * Document-method: Zstd::Parameters#targetlength
 * Document-method: Zstd::Parameters#targetlength=
 * Document-method: Zstd::Parameters#strategy
 * Document-method: Zstd::Parameters#strategy=
 *
 * Get/Set any field from/to struct ZSTD_parameters of C layer.
 */

static void
init_params(void)
{
    extzstd_cParams = rb_define_class_under(extzstd_mZstd, "Parameters", rb_cObject);
    rb_define_alloc_func(extzstd_cParams, params_alloc);
    rb_define_method(extzstd_cParams, "initialize", RUBY_METHOD_FUNC(params_init), -1);
    rb_define_method(extzstd_cParams, "initialize_copy", RUBY_METHOD_FUNC(params_init_copy), 1);
    //rb_define_method(extzstd_cParams, "validate", RUBY_METHOD_FUNC(params_validate), 0);
    //rb_define_method(extzstd_cParams, "srcsize", RUBY_METHOD_FUNC(params_srcsize), 0);
    //rb_define_method(extzstd_cParams, "srcsize=", RUBY_METHOD_FUNC(params_set_srcsize), 1);
    rb_define_method(extzstd_cParams, "windowlog", RUBY_METHOD_FUNC(params_windowlog), 0);
    rb_define_method(extzstd_cParams, "windowlog=", RUBY_METHOD_FUNC(params_set_windowlog), 1);
    rb_define_method(extzstd_cParams, "chainlog", RUBY_METHOD_FUNC(params_chainlog), 0);
    rb_define_method(extzstd_cParams, "chainlog=", RUBY_METHOD_FUNC(params_set_chainlog), 1);
    rb_define_method(extzstd_cParams, "hashlog", RUBY_METHOD_FUNC(params_hashlog), 0);
    rb_define_method(extzstd_cParams, "hashlog=", RUBY_METHOD_FUNC(params_set_hashlog), 1);
    rb_define_method(extzstd_cParams, "searchlog", RUBY_METHOD_FUNC(params_searchlog), 0);
    rb_define_method(extzstd_cParams, "searchlog=", RUBY_METHOD_FUNC(params_set_searchlog), 1);
    rb_define_method(extzstd_cParams, "searchlength", RUBY_METHOD_FUNC(params_searchlength), 0);
    rb_define_method(extzstd_cParams, "searchlength=", RUBY_METHOD_FUNC(params_set_searchlength), 1);
    rb_define_method(extzstd_cParams, "targetlength", RUBY_METHOD_FUNC(params_targetlength), 0);
    rb_define_method(extzstd_cParams, "targetlength=", RUBY_METHOD_FUNC(params_set_targetlength), 1);
    rb_define_method(extzstd_cParams, "strategy", RUBY_METHOD_FUNC(params_strategy), 0);
    rb_define_method(extzstd_cParams, "strategy=", RUBY_METHOD_FUNC(params_set_strategy), 1);

    rb_define_singleton_method(extzstd_cParams, "preset", RUBY_METHOD_FUNC(params_s_get_preset), -1);
    rb_define_alias(rb_singleton_class(extzstd_cParams), "[]", "preset");
}


/*
 * module Zstd::Dictionary
 */

static VALUE mDictionary;

/*
 * call-seq:
 *  train_from_buffer(src, dict_capacity) -> dictionary'd string
 */
static VALUE
dict_s_train_from_buffer(VALUE mod, VALUE src, VALUE dict_capacity)
{
    rb_check_type(src, RUBY_T_STRING);
    size_t capa = NUM2SIZET(dict_capacity);
    VALUE dict = rb_str_buf_new(capa);
    size_t srcsize = RSTRING_LEN(src);
    size_t s = ZDICT_trainFromBuffer(RSTRING_PTR(dict), capa, RSTRING_PTR(src), &srcsize, 1);
    extzstd_check_error(s);
    rb_str_set_len(dict, s);
    return dict;
}

/*
 * call-seq:
 *  add_entropy_tables_from_buffer(dict, dict_capacity, sample) -> dict
 */
static VALUE
dict_s_add_entropy_tables_from_buffer(VALUE mod, VALUE dict, VALUE dict_capacity, VALUE sample)
{
    /*
     * size_t ZDICT_addEntropyTablesFromBuffer(
     *      void* dictBuffer, size_t dictContentSize, size_t dictBufferCapacity,
     *      const void* samplesBuffer, const size_t* samplesSizes, unsigned nbSamples);
     */

    rb_check_type(dict, RUBY_T_STRING);
    rb_check_type(sample, RUBY_T_STRING);
    size_t capa = NUM2SIZET(dict_capacity);
    aux_str_modify_expand(dict, capa);
    size_t samplesize = RSTRING_LEN(sample);
    size_t s = ZDICT_addEntropyTablesFromBuffer(RSTRING_PTR(dict), RSTRING_LEN(dict), capa, RSTRING_PTR(sample), &samplesize, 1);
    extzstd_check_error(s);
    rb_str_set_len(dict, s);
    return dict;
}

static VALUE
dict_s_getid(VALUE mod, VALUE dict)
{
    /*
     * ZDICTLIB_API unsigned ZDICT_getDictID(const void* dictBuffer, size_t dictSize);
     */

    rb_check_type(dict, RUBY_T_STRING);
    const char *p;
    size_t psize;
    RSTRING_GETMEM(dict, p, psize);

    size_t s = ZDICT_getDictID(p, psize);
    extzstd_check_error(s);

    return SIZET2NUM(s);
}

static void
init_dictionary(void)
{
    mDictionary = rb_define_module_under(extzstd_mZstd, "Dictionary");
    rb_define_singleton_method(mDictionary, "train_from_buffer", dict_s_train_from_buffer, 2);
    rb_define_singleton_method(mDictionary, "add_entropy_tables_from_buffer", dict_s_add_entropy_tables_from_buffer, 3);
    rb_define_singleton_method(mDictionary, "getid", dict_s_getid, 1);
}

/*
 * module Zstd::ContextLess
 */

static VALUE mContextLess;

/*
 * call-seq:
 *  encode(src, dest, maxdest, params)
 *
 * [RETURN] dest
 * [src (string)]
 * [dest (string)]
 * [maxdest (integer or nil)]
 * [params (nil, integer or Zstd::Parameters)]
 */
static VALUE
less_s_encode(VALUE mod, VALUE src, VALUE dest, VALUE maxdest, VALUE predict, VALUE params)
{
    const char *q;
    size_t qsize;
    aux_string_pointer(src, &q, &qsize);

    char *r;
    size_t rsize = (NIL_P(maxdest)) ? ZSTD_compressBound(qsize) : NUM2SIZET(maxdest);
    aux_string_expand_pointer(dest, &r, rsize);
    rb_obj_infect(dest, src);

    const char *d;
    size_t dsize;
    aux_string_pointer_with_nil(predict, &d, &dsize);
    rb_obj_infect(dest, predict);

    if (extzstd_params_p(params)) {
        /*
         * ZSTDLIB_API size_t ZSTD_compress_advanced(
         *      ZSTD_CCtx* ctx,
         *      void* dst, size_t dstCapacity,
         *      const void* src, size_t srcSize,
         *      const void* dict,size_t dictSize,
         *      ZSTD_parameters params);
         */
        ZSTD_CCtx *zstd = ZSTD_createCCtx();
        size_t s = ZSTD_compress_advanced(zstd, r, rsize, q, qsize, d, dsize, *extzstd_getparams(params));
        ZSTD_freeCCtx(zstd);
        extzstd_check_error(s);
        rb_str_set_len(dest, s);
        return dest;
    } else {
        /*
         * ZSTDLIB_API size_t ZSTD_compress_usingDict(
         *      ZSTD_CCtx* ctx,
         *      void* dst, size_t dstCapacity,
         *      const void* src, size_t srcSize,
         *      const void* dict,size_t dictSize,
         *      int compressionLevel);
         */
        ZSTD_CCtx *zstd = ZSTD_createCCtx();
        size_t s = ZSTD_compress_usingDict(zstd, r, rsize, q, qsize, d, dsize, aux_num2int(params, 0));
        ZSTD_freeCCtx(zstd);
        extzstd_check_error(s);
        rb_str_set_len(dest, s);
        return dest;
    }
}

/*
 * call-seq:
 *  decode(src, dest, maxdest)
 *
 * [RETURN] dest
 * [src (string)]
 * [dest (string)]
 * [maxdest (integer or nil)]
 */
static VALUE
less_s_decode(VALUE mod, VALUE src, VALUE dest, VALUE maxdest, VALUE predict)
{
    const char *q;
    size_t qsize;
    aux_string_pointer(src, &q, &qsize);

    char *r;
    size_t rsize = (NIL_P(maxdest)) ? ZSTD_getDecompressedSize(q, qsize) : NUM2SIZET(maxdest);
    aux_string_expand_pointer(dest, &r, rsize);
    rb_obj_infect(dest, src);

    const char *d;
    size_t dsize;
    aux_string_pointer_with_nil(predict, &d, &dsize);
    rb_obj_infect(dest, predict);

    ZSTD_DCtx *z = ZSTD_createDCtx();
    size_t s = ZSTD_decompress_usingDict(z, r, rsize, q, qsize, d, dsize);
    ZSTD_freeDCtx(z);
    extzstd_check_error(s);
    rb_str_set_len(dest, s);

    return dest;
}

static void
init_contextless(void)
{
    mContextLess = rb_define_module_under(extzstd_mZstd, "ContextLess");
    rb_define_singleton_method(mContextLess, "encode", less_s_encode, 5);
    rb_define_singleton_method(mContextLess, "decode", less_s_decode, 4);
}

/*
 * library initializer
 */

void
Init_extzstd(void)
{
    extzstd_mZstd = rb_define_module("Zstd");

    init_libver();
    init_error();
    init_constants();
    init_params();
    init_dictionary();
    init_contextless();
    extzstd_init_stream();
}
