/* vim: set ts=4 sts=4 sw=4 noet ai: */
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
