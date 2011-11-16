#include "global.h"

#include "gc.h"
#include "cons.h"
#include "closure.h"
#include "stack.h"
#include "ref.h"

static gc_object_t *first_gc_object = 0;

static int in_sweep_cycle = 0;

static size_t num_roots = 0;
static gc_object_t **gc_roots = 0;

static void _gc_free(gc_object_t *o);
static void _gc_unregister_object(gc_object_t *o);
static void _gc_clear_marks();
static void _gc_mark_roots();
static void _gc_sweep();

/* ref counts in garbage collected objects are stored in a single byte,
   so a value of 255 actually means 255 or more refs */
#define GC_RC_OVERFLOW (255)

void register_gc_root(ref_t ref)
{
	gc_object_t **new_roots = 0;
	gc_object_t *o = 0;

	if (ref.type &&
		ref.type->gc_mark &&
		ref.type->gc_release_refs &&
		ref.type->gc_free_mem)
	{
		o = ref.data.object;
	}
	else
		return; /* it's not a garbage collected type, since it doesn't export those functions */

	assert(o);

	new_roots = (gc_object_t**)X_MALLOC(sizeof(gc_object_t*) * (num_roots + 1));
	memcpy(new_roots, gc_roots, sizeof(gc_object_t*) * num_roots);

	gc_add_ref(o);
	new_roots[num_roots] = o;
	num_roots++;

	if (gc_roots)
		X_FREE(gc_roots);

	gc_roots = new_roots;
}

void unregister_gc_root(ref_t ref)
{
	gc_object_t **new_roots = 0;
	gc_object_t *o = 0;
	size_t i;

	if (num_roots == 0)
	{
		LOG_ERROR("trying to unregister a root when there are no roots.");
		return;
	}

	if (ref.type &&
		ref.type->gc_mark &&
		ref.type->gc_release_refs &&
		ref.type->gc_free_mem)
	{
		o = ref.data.object;
	}
	else
		return; /* it's not a garbage collected type, since it doesn't export those functions */

	assert(o);

	for (i = 0; i < num_roots; ++i)
	{
		if (gc_roots[i] == o)
			break;
	}
	if (i >= num_roots) /* it's not in the root set anyway */
	{
		LOG_ERROR("trying to unregister a root that hasn't been registered");
		return;
	}

	gc_release_ref(gc_roots[i]);

	if (num_roots - 1 > 0)
	{
		new_roots = (gc_object_t**)X_MALLOC(sizeof(gc_object_t*) * (num_roots - 1));

		memcpy(new_roots, gc_roots, sizeof(gc_object_t*) * i);
		memcpy(&new_roots[i], &gc_roots[i+1], sizeof(gc_object_t*) * (num_roots - (i+1)));

		num_roots--;
		X_FREE(gc_roots);
		gc_roots = new_roots;
	}
	else
	{
		num_roots--;
		X_FREE(gc_roots);
		gc_roots = 0;
	}
}

void gc_init_object(gc_object_t *o, const type_traits_t *type)
{
	assert(o);

	o->next_gc_object = first_gc_object;
	first_gc_object = o;

	o->marked = 0; /* TODO: work out what marked should actually be... */
	o->rc = 1;
	o->type = type;
}

void gc_traits_addref(ref_t instance)
{
	gc_add_ref(instance.data.object);
}

void gc_traits_release(ref_t instance)
{
	gc_release_ref(instance.data.object);
}

void gc_add_ref(gc_object_t *o)
{
	assert(o);
	assert(o->rc);

	o->marked = 0;
	/* ref count overflowed; can't use it any more */
	if (o->rc == GC_RC_OVERFLOW)
		return;
	++o->rc;
}

void gc_release_ref(gc_object_t *o)
{
	assert(o && o->type && o->type->gc_mark && o->type->gc_release_refs && o->type->gc_free_mem);

	if (in_sweep_cycle)
	{
		if (o->marked && (o->rc != GC_RC_OVERFLOW))
		{
			--o->rc;
	/* rc cannot go to zero for a marked object during the sweep cycle,
	   because if it would, then that means the object is only visible
	   from another unmarked object which means that it isn't visible
	   which means that it wouldn't be marked in the first place */
			assert(o->rc);
		}

		return;
	}

	assert(o->rc);

	/* ref count overflowed; can't decrement use it any more */
	if (o->rc == GC_RC_OVERFLOW)
		return;

	if (! --o->rc)
	{
		_gc_unregister_object(o);
		_gc_free(o);
	}
}

static void _gc_free(gc_object_t *o)
{
	ref_t ref;

	assert(o && o->type && o->type->gc_mark && o->type->gc_release_refs && o->type->gc_free_mem);

	ref.type = o->type;
	ref.data.object = o;

	o->type->gc_release_refs(ref);
	o->type->gc_free_mem(ref);
}

static void _gc_clear_marks()
{
	gc_object_t *o = first_gc_object;
	while (o)
	{
		o->marked = 0;
		o = o->next_gc_object;
	}
}

static void _gc_sweep_refs(gc_object_t *o)
{
	ref_t ref;

	assert(o && o->type && o->type->gc_mark && o->type->gc_release_refs && o->type->gc_free_mem);

	ref.type = o->type;
	ref.data.object = o;

	o->type->gc_release_refs(ref);
}

static void _gc_sweep_mem(gc_object_t *o)
{
	ref_t ref;

	assert(o && o->type && o->type->gc_mark && o->type->gc_release_refs && o->type->gc_free_mem);

	ref.type = o->type;
	ref.data.object = o;

	o->type->gc_free_mem(ref);
}

static void _gc_sweep()
{
	gc_object_t *o, **prev, *free_list = 0;

	in_sweep_cycle = 1;

	/* phase 1 - walk through the list of objects.  for any that are marked,
	   let them decrement the ref counts of whatever they reference, remove them
	   from the object list and add them to the free list */

	o = first_gc_object;
	prev = &first_gc_object;
	while (o)
	{
		gc_object_t *next = o->next_gc_object;
		if (! o->marked)
		{
			*prev = next;

			_gc_sweep_refs(o);
			o->next_gc_object = free_list;
			free_list = o;
		}
		else
			prev = &o->next_gc_object;
		o = next;
	}

	/* phase 2 -  */
	o = free_list;
	while (o)
	{
		gc_object_t *next = o->next_gc_object;
		_gc_sweep_mem(o);
		o = next;
	}

	in_sweep_cycle = 0;
}

static void _gc_mark_roots()
{
	size_t i;
	stack_gc_mark_root();
	for (i = 0; i < num_roots; ++i)
		gc_mark(gc_roots[i]);
}

static void _gc_unregister_object(gc_object_t *obj)
{
	if (first_gc_object == obj)
	{
		first_gc_object = obj->next_gc_object;
		obj->next_gc_object = 0;
	}
	else
	{
		gc_object_t *o = first_gc_object;
		while (o)
		{
			if (o->next_gc_object == obj)
			{
				o->next_gc_object = obj->next_gc_object;
				obj->next_gc_object = 0;
				break;
			}
			o = o->next_gc_object;
		}
	}
}

void gc_mark(gc_object_t *o)
{
	ref_t ref;

	assert(o && o->type && o->type->gc_mark && o->type->gc_release_refs && o->type->gc_free_mem);

	if (o->marked)
		return;

	o->marked = 1;

	ref.type = o->type;
	ref.data.object = o;

	o->type->gc_mark(ref);
}

void collect_garbage()
{
	_gc_clear_marks();
	_gc_mark_roots();
	_gc_sweep();
}
