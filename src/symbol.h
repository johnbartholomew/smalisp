/* vim: set ts=4 sts=4 sw=4 noet ai: */
#ifndef SYMBOL_H
#define SYMBOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vector.h"
#include "sl_string.h"
#include "stack.h"

/* nb: symbols do contain references to other objects, but do not have to be handled by the,
   mark/sweep garbage collector, because they only contain weak references,
   so they can't create cycles.  The strong references are held in the stack frame objects;
   the references that symbols hold in their value stack is just a way of caching the data */
struct symbol_ts
{
	unsigned long rc;
	string_t *name;
	vector_t binding_stack;
};

void symbol_let(ref_t symbol, ref_t value, size_t frame);
void symbol_set(ref_t symbol, ref_t new_value, size_t start_frame);
void symbol_unset(ref_t symbol, size_t frame);

extern int symbol_eval_count;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
