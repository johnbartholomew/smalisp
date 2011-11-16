#include "global.h"

#include "smalisp.h"
#include "gc.h"
#include "ref.h"
#include "symbol.h"

#include "stack_frame.h"
#include "stack.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

int stack_switch_count = 0;

static stack_t *current_stack = 0;

static void stack_traits_gc_mark(ref_t instance)
{
	stack_t *s = (stack_t*)instance.data.object;
	stack_frame_t **it, **end;

	assert(s);

	it = (stack_frame_t**)s->frames.items;
	end = (stack_frame_t**)vector_end(&s->frames);
	while (it != end)
	{
		stack_frame_t *sf = *it;
		gc_mark(&sf->gc);
		++it;
	}
}

static void stack_traits_gc_release_refs(ref_t instance)
{
	stack_t *s = (stack_t*)instance.data.object;
	stack_frame_t **it, **end;

	assert(s);

	it = (stack_frame_t**)s->frames.items;
	end = (stack_frame_t**)vector_end(&s->frames);
	while (it != end)
	{
		stack_frame_t *sf = *it;
		gc_release_ref(&sf->gc);
		++it;
	}
}

static void stack_traits_gc_free_mem(ref_t instance)
{
	stack_t *s = (stack_t*)instance.data.object;

	assert(s);

	vector_clear(&s->frames);
	X_FREE(s);
}

static void stack_traits_print(ref_t instance, FILE *to)
{
	stack_t *s = (stack_t*)instance.data.object;
	assert(s);
	fprintf(to, "#<stack %p>", s);
}

static ref_t stack_traits_type_name(ref_t instance)
{
	return make_symbol("stack", 0);
}

static int stack_traits_eq(ref_t a, ref_t b)
{
	return a.data.object == b.data.object;
}

static const type_traits_t stack_traits =
{
	0, /* not evaluable */
	0, /* not executable */
	stack_traits_print,
	stack_traits_type_name,
	stack_traits_eq,
	stack_traits_eq, /* eq and eql do the same thing for stacks */
	gc_traits_addref,
	gc_traits_release,
	stack_traits_gc_mark,
	stack_traits_gc_release_refs,
	stack_traits_gc_free_mem
};
const type_traits_t *stack_type = &stack_traits;

static stack_frame_t *top_frame(stack_t *s)
{
	stack_frame_t **sf = (stack_frame_t**)vector_back(&s->frames);
	if (!sf)
		return 0;
	else
		return *sf;
}

static int stack_visible(stack_t *target)
{
	stack_t *s;
	s = current_stack;
	while (s)
	{
		if (s == target)
			return -1;

		s = s->parent;
	}

	return 0;
}

ref_t make_stack(ref_t parent)
{
	ref_t ref;
	stack_frame_t **new_frame;
	stack_t *s = (stack_t*)X_MALLOC(sizeof(stack_t));
	gc_init_object(&s->gc, stack_type);

	if (parent.type == NIL)
	{
		s->parent = 0;
		VECTOR_INIT_TYPE(&s->frames, stack_frame_t**);
	}
	else if (parent.type == stack_type)
	{
		stack_frame_t **it, **end;
		s->parent = (stack_t*)parent.data.object;

		VECTOR_INIT_TYPE(&s->frames, stack_frame_t**);
		vector_reserve(&s->frames, s->parent->frames.size + 1);

		s->frames.size = s->parent->frames.size;
		memcpy(s->frames.items, s->parent->frames.items, s->parent->frames.item_size * s->parent->frames.size);

		it = (stack_frame_t**)s->frames.items;
		end = (stack_frame_t**)vector_end(&s->frames);
		while (it != end)
		{
			stack_frame_t *sf = *it;
			gc_add_ref(&sf->gc);

			++it;
		}
	}
	else
	{
		LOG_ERROR("Called with an invalid parent; not a stack");
		return nil();
	}

	new_frame = (stack_frame_t**)vector_insert(&s->frames, VECTOR_NPOS);
	*new_frame = make_stack_frame();

	ref.type = stack_type;
	ref.data.object = &s->gc;
	return ref;
}

static int _stack_frame_eq(void *sf, void *target)
{
	stack_frame_t **a = (stack_frame_t**)sf;
	stack_frame_t **b = (stack_frame_t**)target;
	return *a == *b;
}

static void _stack_set(stack_t *s, ref_t name, ref_t val)
{
	stack_frame_t **it, **front;

	it = (stack_frame_t**)vector_end(&s->frames);
	front = (stack_frame_t**)s->frames.items;
	while (it != front)
	{
		stack_slot_t *slot;
		--it;

		slot = stack_frame_find(*it, name, 0);
		if (slot)
		{
			size_t frame_id;

			ref_t old_val = slot->value;
			slot->value = clone_ref(val);
			release_ref(&old_val);

			if (s == current_stack)
				frame_id = vector_idx_from_it(&s->frames, it);
			else
				frame_id = vector_findr(&current_stack->frames, it, _stack_frame_eq);

			if (frame_id != VECTOR_NPOS)
				symbol_set(name, val, frame_id);

			return;
		}
	}

	/* we didn't find it */
	LOG_ERROR("Didn't find given name in the stack (so it could not be rebound)");
}

void stack_set(ref_t stack, ref_t name, ref_t val)
{
	stack_t *s = (stack_t*)stack.data.object;
	assert(s);

	if (name.type != symbol_type)
	{
		LOG_ERROR("Called with name not a symbol.");
		return;
	}

	_stack_set(s, name, val);
}

static void _stack_let(stack_t *s, ref_t name, ref_t val)
{
	ref_t old_val;
	stack_slot_t *slot;
	size_t frame_id;
	stack_frame_t **it, *sf;
	it = (stack_frame_t**)vector_back(&s->frames);
	sf = *it;

	assert(sf);

	slot = stack_frame_find(sf, name, 1);
	old_val = slot->value;
	slot->value = clone_ref(val);
	release_ref(&old_val);

	if (s == current_stack)
		frame_id = s->frames.size - 1;
	else
		frame_id = vector_findr(&current_stack->frames, sf, _stack_frame_eq);

	if (frame_id != VECTOR_NPOS)
		symbol_let(name, val, frame_id);
}

void stack_let(ref_t stack, ref_t name, ref_t val)
{
	stack_t *s = (stack_t*)stack.data.object;
	assert(s);

	if (name.type != symbol_type)
	{
		LOG_ERROR("Called with name not a symbol.");
		return;
	}

	_stack_let(s, name, val);
}

static void _stack_enter(stack_t *s)
{
	size_t num_common, frame_id;
	stack_frame_t **cur, **to, **it, **end;
	stack_t *old_stack = current_stack;

	if (s == current_stack)
		return;

	num_common = 0;

	if (current_stack && s)
	{
		/* step one: iterate through the two stacks from
		   front to back to find out how many stack frames are shared */
		cur = (stack_frame_t**)current_stack->frames.items;
		to = (stack_frame_t**)s->frames.items;
		while (*to == *cur)
		{
			++num_common;
			++to;
			++cur;
		}
	}

	if (current_stack)
	{
		/* step two: iterate backwards through the current stack to
		   wipe out symbol bindings that are no longer applicable */
		it = (stack_frame_t**)vector_end(&current_stack->frames);
		end = (stack_frame_t**)vector_nth(&current_stack->frames, num_common);
		frame_id = current_stack->frames.size;
		while (it != end)
		{
			--it;
			--frame_id;
			stack_frame_pop_bindings(*it, frame_id);
		}
	}

	if (s)
	{
		/* step three: iterate forwards through the new stack to
		   add symbol bindings that are now visible */
		it = (stack_frame_t**)vector_nth(&s->frames, num_common);
		end = (stack_frame_t**)vector_end(&s->frames);
		frame_id = num_common;
		while (it != end)
		{
			stack_frame_push_bindings(*it, frame_id);
			++it;
			++frame_id;
		}

		/* step four, set the current frame pointer */
		gc_add_ref(&s->gc);
	}

	++stack_switch_count;
	current_stack = s;

	if (old_stack)
		gc_release_ref(&old_stack->gc);
}

void stack_enter(ref_t stack)
{
	stack_t *s;
	if (stack.type == NIL)
		_stack_enter(0);
	else
	{
		s = (stack_t*)stack.data.object;
		assert(s);
		_stack_enter(s);
	}
}

void stack_debug_print(stack_t *s, FILE *to)
{
	stack_frame_t **it, **front;

	assert(s);

	fprintf(to, "### Stack dump for stack %p:\n", s);
	
	front = (stack_frame_t**)s->frames.items;
	it = (stack_frame_t**)vector_end(&s->frames);
	while (it != front)
	{
		--it;

		stack_frame_debug_print(*it, to);
		fprintf(to, "\n");
	}
	fprintf(to, "#################\n");
}

void stack_gc_mark_root()
{
	if (current_stack)
		gc_mark(&current_stack->gc);
}
