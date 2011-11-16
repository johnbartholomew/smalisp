#ifndef VECTOR_H
#define VECTOR_H

typedef struct vector_ts vector_t;

struct vector_ts
{
	size_t item_size;
	size_t size;
	size_t capacity;
	char *items;
};

#define VECTOR_NPOS ((size_t)(-1))

/* construction/destruction */
vector_t vector_init(vector_t *vec, size_t item_size);
#define VECTOR_INIT_TYPE(v, t) vector_init((v), sizeof(t))
vector_t vector_clone(vector_t *dst, vector_t *src);
void vector_clear(vector_t *vec);

#define VECTOR_STATIC_INIT(t) { sizeof(t), 0, 0, 0 }

/* accessors */
void vector_compact(vector_t *vec);
void *vector_nth(vector_t *vec, size_t n);
void *vector_back(vector_t *vec);
void *vector_begin(vector_t *vec);
void *vector_end(vector_t *vec);
size_t vector_idx_from_it(vector_t *vec, void *it);
size_t vector_find(vector_t *vec, void *target, int(*eq)(void*, void*));
size_t vector_findr(vector_t *vec, void *target, int(*eq)(void*, void*));
void vector_traverse(vector_t *vec, void(*op)(void*));

/* manipulators */
void vector_reserve(vector_t *vec, size_t new_size);
/* inserts an item into the vector; if pos is -1, then it will be inserted at the back of the vector
   nb: it returns the pointer to the new item (which is at index pos); it does not clear this memory
   so it may still contain whatever item was in that location previously. */
void *vector_insert(vector_t *vec, size_t pos);
void vector_erase(vector_t *vec, size_t pos);
void vector_erase_back(vector_t *vec);

#endif
