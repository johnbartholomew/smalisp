#ifndef STACK_FRAME_H
#define STACK_FRAME_H

#include "gc.h"
#include "ref.h"
#include "vector.h"

#ifdef __cplusplus
extern "C" {
#endif

void stack_frame_global_init();
void stack_frame_global_cleanup();

typedef struct stack_frame_ts stack_frame_t;
struct stack_frame_ts
{
	gc_object_t gc;
	vector_t items; /* a vector of stack slots */
};

typedef struct stack_slot_ts stack_slot_t;
struct stack_slot_ts
{
	ref_t symbol;
	ref_t value;
};

stack_frame_t *make_stack_frame();

void stack_frame_debug_print(stack_frame_t *sf, FILE *to);

stack_slot_t *stack_frame_find(stack_frame_t *sf, ref_t name, int insert);
void stack_frame_erase(stack_frame_t *sf, ref_t name);

void stack_frame_pop_bindings(stack_frame_t *sf, size_t use_id);
void stack_frame_push_bindings(stack_frame_t *sf, size_t use_id);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
