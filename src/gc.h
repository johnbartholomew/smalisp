/* vim: set ts=4 sts=4 sw=4 noet ai: */
#ifndef GC_H
#define GC_H

#include "smalisp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long refcount_t;

struct gc_object_ts
{
	struct gc_object_ts *next_gc_object;
	byte_t rc;
	byte_t marked;
	const type_traits_t *type;
};

void gc_init_object(gc_object_t *o, const type_traits_t *type); /* initializes an object with a ref count of 1 */

void gc_add_ref(gc_object_t *o);
void gc_release_ref(gc_object_t *o);
void gc_mark(gc_object_t *o);

void gc_traits_addref(ref_t instance);
void gc_traits_release(ref_t instance);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
