#ifndef EXTZSTD_H
#define EXTZSTD_H 1

#include <ruby.h>
#include <zstd.h>
#include <zstd_static.h>

extern VALUE mZstd;
extern VALUE cStreamEncoder;
extern VALUE cStreamDecoder;
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

#endif /* !EXTZSTD_H */
