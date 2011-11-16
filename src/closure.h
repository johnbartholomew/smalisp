#ifndef CLOSURE_H
#define CLOSURE_H

#include "gc.h"
#include "ref.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct closure_ts
{
	gc_object_t gc;
	ref_t param_list;
	ref_t code;
	ref_t env;
} closure_t;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
