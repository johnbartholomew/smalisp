#ifndef CONS_H
#define CONS_H

#include "gc.h"
#include "ref.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cons_ts
{
	gc_object_t gc;
	ref_t car;
	ref_t cdr;
} cons_t;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
