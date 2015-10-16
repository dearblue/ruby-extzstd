#ifndef EXTZSTD_H
#define EXTZSTD_H 1

#include <stdarg.h>
#include <ruby.h>
#include <ruby/thread.h>
#include <zstd.h>
#include <zstd_static.h>

#define RDOCFAKE(DUMMY_CODE)

extern VALUE mZstd;
extern VALUE eError;

void init_extzstd_stream(void);

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

static inline VALUE
aux_str_modify_expand(VALUE s, size_t z)
{
    rb_check_type(s, RUBY_T_STRING);
    size_t size = RSTRING_LEN(s);
    if (z > size) {
        rb_str_modify_expand(s, z - size);
    } else {
        rb_str_modify(s);
    }
    return s;
}

static inline void *
aux_thread_call_without_gvl(void *(*func)(void *), void (*cancel)(void *), ...)
{
    va_list va1, va2;
    va_start(va1, cancel);
    va_start(va2, cancel);
    void *p = rb_thread_call_without_gvl(func, &va1, cancel, &va2);
    va_end(va1);
    va_end(va2);
    return p;
}

#endif /* !EXTZSTD_H */
