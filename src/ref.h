#ifndef REF_H
#define REF_H

#include "smalisp.h"

#ifdef __cplusplus
extern "C" {
#endif

void ref_gc_mark(ref_t ref);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
