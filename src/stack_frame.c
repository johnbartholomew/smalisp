#include "global.h"

#include "smalisp.h"
#include "gc.h"
#include "ref.h"
#include "stack_frame.h"
#include "symbol.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

stack_frame_t *make_stack_frame()
{
	stack_frame_t *sf = (stack_frame_t*)X_MALLOC(sizeof(stack_frame_t));
	gc_init_object(&sf->gc, stack_frame_type);
	if (! sf)
	{
		LOG_ERROR("Memory allocation error");
		return 0;
	}
	VECTOR_INIT_TYPE(&sf->items, stack_slot_t);
	return sf;
}

stack_slot_t *stack_frame_find(stack_frame_t *sf, ref_t name, int insert)
{
	stack_slot_t *it, *end;
	
	assert(sf);

	it = (stack_slot_t*)sf->items.items;
	end = (stack_slot_t*)vector_end(&sf->items);
	while (it != end)
	{
		if (eq(it->symbol, name))
			return it;

		++it;
	}

	if (!insert)
		return 0;

	it = (stack_slot_t*)vector_insert(&sf->items, VECTOR_NPOS);
	it->symbol = clone_ref(name);
	it->value = nil();
	return it;
}

void stack_frame_erase(stack_frame_t *sf, ref_t name)
{
	stack_slot_t *it, *end;
	size_t n;

	assert(sf);

	n = 0;
	it = (stack_slot_t*)sf->items.items;
	end = (stack_slot_t*)vector_end(&sf->items);
	while (it != end)
	{
		if (eq(it->symbol, name))
		{
			vector_erase(&sf->items, n);
			return;
		}

		++it;
		++n;
	}
}

static void stack_frame_traits_gc_mark(ref_t instance)
{
	stack_frame_t *sf = (stack_frame_t*)instance.data.object;
	stack_slot_t *it, *end;

	assert(sf);

	it = (stack_slot_t*)sf->items.items;
	end = (stack_slot_t*)vector_end(&sf->items);
	while (it != end)
	{
		ref_gc_mark(it->symbol);
		ref_gc_mark(it->value);

		++it;
	}
}

static void stack_frame_traits_gc_release_refs(ref_t instance)
{
	stack_frame_t *sf = (stack_frame_t*)instance.data.object;
	stack_slot_t *it, *end;

	assert(sf);

	it = (stack_slot_t*)sf->items.items;
	end = (stack_slot_t*)vector_end(&sf->items);
	while (it != end)
	{
		release_ref(&it->symbol);
		release_ref(&it->value);

		++it;
	}
}

static void stack_frame_traits_gc_free_mem(ref_t instance)
{
	stack_frame_t *sf = (stack_frame_t*)instance.data.object;

	assert(sf);

	vector_clear(&sf->items);
	X_FREE(sf);
}

static const type_traits_t stack_frame_traits =
{
	0, /* not evaluable */
	0, /* not executable */
	0, /* not printable */
	0, /* no type name (hidden type) */
	0, /* no eq (hidden type) */
	0, /* no eql (hidden type) */
	gc_traits_addref,
	gc_traits_release,
	stack_frame_traits_gc_mark,
	stack_frame_traits_gc_release_refs,
	stack_frame_traits_gc_free_mem
};
const type_traits_t *stack_frame_type = &stack_frame_traits;

void stack_frame_debug_print(stack_frame_t *sf, FILE *to)
{
	stack_slot_t *it, *end;

	assert(sf);

	fprintf(to, "### Frame %p:\n", sf);
	
	it = (stack_slot_t*)sf->items.items;
	end = (stack_slot_t*)vector_end(&sf->items);
	while (it != end)
	{
		print(it->symbol, to);
		fprintf(to, " -> ");
		print(it->value, to);
		fprintf(to, "\n");

		++it;
	}
}

void stack_frame_pop_bindings(stack_frame_t *sf, size_t use_id)
{
	stack_slot_t *it, *end;
	assert(sf);

	it = (stack_slot_t*)vector_end(&sf->items);
	end = (stack_slot_t*)sf->items.items;
	while (it != end)
	{
		--it;
		symbol_unset(it->symbol, use_id);
	}
}

void stack_frame_push_bindings(stack_frame_t *sf, size_t use_id)
{
	stack_slot_t *it, *end;
	assert(sf);

	it = (stack_slot_t*)sf->items.items;
	end = (stack_slot_t*)vector_end(&sf->items);
	while (it != end)
	{
		symbol_let(it->symbol, it->value, use_id);
		++it;
	}
}
