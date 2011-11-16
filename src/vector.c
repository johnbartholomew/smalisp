/* vim: set ts=4 sts=4 sw=4 noet ai: */
#include "global.h"

#include "vector.h"

static void _vector_resize_buffer(vector_t *vec, size_t req_size)
{
	size_t nsize = vec->capacity ? vec->capacity : 1;
	char *nitems;
	
	assert(vec && vec->item_size);

	if (req_size <= vec->capacity)
		return;

	while (nsize < req_size)
		nsize <<= 1;

	nitems = (char*)X_MALLOC(vec->item_size * nsize);
	if (! nitems)
	{
		LOG_ERROR("Memory allocation error.");
		return;
	}

	if (vec->size)
	{
		assert(vec->items);

		memcpy(nitems, vec->items, vec->item_size * vec->size);
		X_FREE(vec->items);
	}

	vec->items = nitems;
	vec->capacity = nsize;
}

vector_t vector_init(vector_t *vec, size_t item_size)
{
	static vector_t tmp;
	if (!vec)
		vec = &tmp;

	vec->item_size = item_size;
	vec->capacity = 0;
	vec->size = 0;
	vec->items = 0;

	return *vec;
}

vector_t vector_clone(vector_t *dst, vector_t *src)
{
	static vector_t tmp;
	if (!dst)
		dst = &tmp;

	dst->item_size = src->item_size;
	dst->capacity = src->size;
	dst->size = src->size;

	if (dst->size)
	{
		dst->items = (char*)X_MALLOC(src->item_size * src->size);
		if (! dst->items)
		{
			LOG_ERROR("Memory allocation error.");
			return *dst;
		}
		memcpy(dst->items, src->items, src->item_size * src->size);
	}
	else
		dst->items = 0;

	return *dst;
}

void vector_clear(vector_t *vec)
{
	assert(vec && vec->item_size);

	if (vec->items)
		X_FREE(vec->items);
	vec->items = 0;
	vec->size = 0;
	vec->capacity = 0;
}

/* accessors */
void vector_compact(vector_t *vec)
{
	char *nitems = 0;

	assert(vec && vec->item_size);

	if (vec->capacity <= vec->size)
		return;

	if (vec->size)
	{
		assert(vec->items);

		nitems = (char*)X_MALLOC(vec->item_size * vec->size);
		memcpy(nitems, vec->items, vec->item_size * vec->size);

		X_FREE(vec->items);
	}

	vec->items = nitems;
	vec->capacity = vec->size;
}

void *vector_nth(vector_t *vec, size_t n)
{
	assert(vec && vec->item_size);
	assert(n <= vec->size); /* <= because we may wish to get the end iterator; ie, one past the last item */

	return (void*)(vec->items + (vec->item_size * n));
}

void *vector_back(vector_t *vec)
{
	assert(vec && vec->item_size);

	if (vec->size == 0)
		return 0;
	else
		return (void*)(vec->items + (vec->item_size * (vec->size - 1)));
}

void *vector_begin(vector_t *vec)
{
	assert(vec && vec->item_size);
	return vec->items;
}

void *vector_end(vector_t *vec)
{
	assert(vec && vec->item_size);
	return vec->items + (vec->item_size * vec->size);
}

size_t vector_idx_from_it(vector_t *vec, void *it)
{
	size_t i = (size_t)it;

	assert(vec && vec->item_size);

	i -= (size_t)(vec->items);
	i /= vec->item_size;
	return i;
}

size_t vector_find(vector_t *vec, void *target, int(*eq)(void*, void*))
{
	size_t n;
	char *it;

	assert(vec && vec->item_size);

	if (vec->size == 0)
		return VECTOR_NPOS;

	n = 0;
	it = vec->items;

	while (n < vec->size)
	{
		if (eq((void*)it, target))
			return n;

		++n;
		it += vec->item_size;
	}

	return VECTOR_NPOS;
}

size_t vector_findr(vector_t *vec, void *target, int(*eq)(void*, void*))
{
	size_t n;
	char *it;

	assert(vec && vec->item_size);

	if (vec->size == 0)
		return VECTOR_NPOS;

	n = vec->size;
	it = (char*)vector_end(vec);

	while (n > 0)
	{
		it -= vec->item_size;
		--n;

		if (eq((void*)it, target))
			return n;
	}

	return VECTOR_NPOS;
}

void vector_traverse(vector_t *vec, void (*op)(void *))
{
	char *it, *end;

	assert(vec && vec->item_size);

	if (vec->size == 0)
		return;

	it = vec->items;
	end = vector_end(vec);
	while (it != end)
	{
		op(it);

		it += vec->item_size;
	}
}

/* manipulators */
void vector_reserve(vector_t *vec, size_t new_size)
{
	char *nitems;

	assert(vec && vec->item_size);

	/* don't want to use the normal resize function here, because we're being given an
	   exact capacity to grow to; we don't want to grow to the next power of two up.
	   So, if an application knows how many items the vector is going to hold, then it
	   can call vector_reserve, and it won't waste memory */

	if (new_size <= vec->capacity)
		return;

	nitems = (char*)X_MALLOC(vec->item_size * new_size);
	if (vec->size)
	{
		assert(vec->items);

		memcpy(nitems, vec->items, vec->item_size * vec->size);
		X_FREE(vec->items);
	}
	
	vec->items = nitems;
	vec->capacity = new_size;
}

/* inserts an item into the vector; if pos is -1, then it will be inserted at the back of the vector */
void *vector_insert(vector_t *vec, size_t pos)
{
	assert(vec && vec->item_size);

	_vector_resize_buffer(vec, vec->size + 1);
	if (pos == VECTOR_NPOS || pos == vec->size)
	{
		char *it = vec->items + (vec->item_size * vec->size);
		++vec->size;
		return (void*)it;
	}
	else
	{
		char *it = vec->items + (vec->item_size * pos);
		char *dst;

		assert(pos < vec->size);
		for (dst = vector_back(vec); dst > it; dst -= vec->item_size)
			memcpy(dst, dst - vec->item_size, vec->item_size);

		return (void*)it;
	}
}

void vector_erase(vector_t *vec, size_t pos)
{
	char *dst, *back;

	assert(vec && vec->item_size);
	assert(vec->size > 0);
	assert(pos < vec->size);

	back = vector_back(vec);
	for (dst = vector_nth(vec, pos); dst < back; dst += vec->item_size)
		memcpy(dst, dst + vec->item_size, vec->item_size);

	--vec->size;
}

void vector_erase_back(vector_t *vec)
{
	assert(vec && vec->item_size);
	if (vec->size)
		--vec->size;
}
