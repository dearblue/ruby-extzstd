#ifndef EXTZSTD_H
#define EXTZSTD_H 1

#define ZSTD_LEGACY_SUPPORT 1
#define ZDICT_STATIC_LINKING_ONLY 1
//#define ZSTD_STATIC_LINKING_ONLY 1
#include <common/zstd_internal.h> /* for MIN() */
#include <zstd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ruby.h>
#include <ruby/thread.h>
#include <ruby/version.h>

#define ELEMENTOF(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))
#define ENDOF(ARRAY) (ARRAY + ELEMENTOF(ARRAY))

#define LOGF(MESG, ...)                                         \
    do {                                                        \
        fprintf(stderr, "%s:%d:%s: " MESG "\n",                 \
                __FILE__, __LINE__, __func__, ## __VA_ARGS__);  \
    } while (0)                                                 \

#define RDOCFAKE(DUMMY_CODE)

extern VALUE extzstd_mZstd;
RDOCFAKE(extzstd_mZstd = rb_define_module("Zstd"));

extern VALUE extzstd_cParams;
RDOCFAKE(extzstd_cParams = rb_define_class_under(extzstd_mZstd, "Parameters", rb_cObject));

extern VALUE extzstd_mExceptions;
extern VALUE extzstd_eError;

extern void init_extzstd_stream(void);
extern void extzstd_init_buffered(void);
extern void extzstd_init_stream(void);
extern void extzstd_error(ssize_t errcode);
extern void extzstd_check_error(ssize_t errcode);
extern VALUE extzstd_make_error(ssize_t errcode);
extern VALUE extzstd_make_errorf(ssize_t errcode, const char *fmt, ...);

extern ZSTD_parameters *extzstd_getparams(VALUE v);
extern int extzstd_params_p(VALUE v);
extern VALUE extzstd_params_alloc(ZSTD_parameters **p);

static inline void
referror(VALUE v)
{
    rb_raise(rb_eRuntimeError,
             "invalid reference - not initialized yet (#<%s:%p>)",
             rb_obj_classname(v), (void *)v);
}

static inline void
reiniterror(VALUE v)
{
    rb_raise(rb_eRuntimeError,
             "already initialized (#<%s:%p>)",
             rb_obj_classname(v), (void *)v);
}

static inline void *
checkref(VALUE v, void *p)
{
    if (!p) { referror(v); }
    return p;
}

static inline void *
getrefp(VALUE v, const rb_data_type_t *type)
{
    void *p;
    TypedData_Get_Struct(v, void, type, p);
    return p;
}

static inline void *
getref(VALUE v, const rb_data_type_t *type)
{
    return checkref(v, getrefp(v, type));
}

#define AUX_IMPLEMENT_CONTEXT(TYPE, TYPEDDATANAME, STRUCTNAME, ALLOCNAME,   \
                              MARKFUNC, FREEFUNC, SIZEFUNC,                 \
                              GETREFP, GETREF, REF_P)                       \
    static const rb_data_type_t TYPEDDATANAME = {                           \
        .wrap_struct_name = STRUCTNAME,                                     \
        .function.dmark = MARKFUNC,                                         \
        .function.dfree = FREEFUNC,                                         \
        .function.dsize = SIZEFUNC,                                         \
    };                                                                      \
                                                                            \
    static VALUE                                                            \
    ALLOCNAME(VALUE klass)                                                  \
    {                                                                       \
        return TypedData_Wrap_Struct(klass, &TYPEDDATANAME, NULL);          \
    }                                                                       \
                                                                            \
    static inline TYPE *                                                    \
    GETREFP(VALUE v)                                                        \
    {                                                                       \
        return getrefp(v, &TYPEDDATANAME);                                  \
    }                                                                       \
                                                                            \
    static inline TYPE *                                                    \
    GETREF(VALUE v)                                                         \
    {                                                                       \
        return getref(v, &TYPEDDATANAME);                                   \
    }                                                                       \
                                                                            \
    static inline int                                                       \
    REF_P(VALUE v)                                                          \
    {                                                                       \
        return rb_typeddata_is_kind_of(v, &TYPEDDATANAME);                  \
    }                                                                       \

static inline VALUE
aux_str_modify_expand(VALUE str, size_t size)
{
    size_t capa = rb_str_capacity(str);
    if (capa < size) {
        rb_str_modify_expand(str, size - RSTRING_LEN(str));
    } else {
        rb_str_modify(str);
    }
    return str;
}

static inline int
aux_num2int(VALUE v, int default_value)
{
    if (NIL_P(v)) {
        return default_value;
    } else {
        return NUM2INT(v);
    }
}

static inline uint64_t
aux_num2int_u64(VALUE v, uint64_t default_value)
{
    if (NIL_P(v)) {
        return default_value;
    } else {
        return NUM2ULL(v);
    }
}

static inline VALUE
aux_const_dig_str_0(VALUE obj, const char *p[], const char **pp)
{
    for (; p < pp; p ++) {
        obj = rb_const_get(obj, rb_intern(*p));
    }
    return obj;
}

#define aux_const_dig_str(OBJ, ...)                         \
        aux_const_dig_str_0((OBJ),                          \
                ((const char *[]){ __VA_ARGS__ }),          \
                ENDOF(((const char *[]){ __VA_ARGS__ })))   \

#define AUX_TUPLE(...)                                  \
    rb_ary_new4(ELEMENTOF(((VALUE[]) { __VA_ARGS__ })), \
            ((VALUE[]) { __VA_ARGS__ }))                \

#define AUX_FUNCALL(RECV, MID, ...)                         \
    rb_funcall2((RECV), (MID),                              \
            ELEMENTOF(((const VALUE[]){ __VA_ARGS__ })),    \
            ((const VALUE[]){ __VA_ARGS__ }))               \

#define AUX_TRY_WITH_GC(cond, mesg) \
    do {                            \
        if (!(cond)) {              \
            rb_gc();                \
            if (!(cond)) {          \
                errno = ENOMEM;     \
                rb_sys_fail(mesg);  \
            }                       \
        }                           \
    } while (0)                     \

#if defined _WIN32 || defined __CYGWIN__
#   define RBEXT_IMPORT __declspec(dllimport)
#   define RBEXT_EXPORT __declspec(dllexport)
#   define RBEXT_LOCAL
#elif defined(__GNUC__) && __GNUC__ >= 4 || defined(__clang__)
#   define RBEXT_IMPORT __attribute__((visibility("default")))
#   define RBEXT_EXPORT __attribute__((visibility("default")))
#   define RBEXT_LOCAL  __attribute__((visibility("hidden")))
#else
#   define RBEXT_IMPORT
#   define RBEXT_EXPORT
#   define RBEXT_LOCAL
#endif

#ifndef RBEXT_API
#   define RBEXT_API RBEXT_EXPORT
#endif


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

#if RUBY_API_VERSION_CODE >= 30000
# define rb_obj_infect(dest, src) ((void)(dest), (void)(src))
#endif

#endif /* EXTZSTD_H */
