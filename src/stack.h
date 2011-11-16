/* vim: set ts=4 sts=4 sw=4 noet ai: */
#ifndef STACK_H
#define STACK_H

#include "gc.h"
#include "ref.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stack_ts stack_t;
struct stack_ts
{
	gc_object_t gc;
	vector_t frames; /* a vector of stack frames */
	stack_t *parent;
};

void stack_debug_print(stack_t *s, FILE *to);
void stack_gc_mark_root();

extern int stack_switch_count;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
