/* vim: set ts=4 sts=4 sw=4 noet ai: */
#include "global.h"

#undef malloc
#undef free

#include "rbt.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

#define MEMCAST(x,type) (*((type*)(&(x))))

static int first_run = 1;

static int manipulating_block_tree = 0;
#define NUM_BLOCK_TREE_ROOTS (32)
static rbtn_t *block_tree_roots[NUM_BLOCK_TREE_ROOTS] = {0};

static int leaked_block_count = 0;
static int alloc_count = 0;

static FILE *log_file = 0;

typedef struct block_info_ts block_info_t;
struct block_info_ts
{
	int alloc_count;
	void *p;
	const char *func;
	const char *file;
	unsigned long line;
	const char *msg;
	unsigned int seq;
	block_info_t *next;
};

static block_info_t *first_block_info = 0;
static block_info_t *last_block_info = 0;

#define ROOT(p) (&(block_tree_roots[(MEMCAST(p, unsigned int) >> 2) % NUM_BLOCK_TREE_ROOTS]))

static block_info_t *_add_new_block_info(void *p, const char *func, const char *file, unsigned long line, const char *msg)
{
	block_info_t *info;
		
	info = (block_info_t*)malloc(sizeof(block_info_t));

	info->next = 0;
	if (last_block_info)
		last_block_info->next = info;
	last_block_info = info;

	if (! first_block_info)
		first_block_info = info;

	info->p = p;
	info->alloc_count = 0;
	info->file = file;
	info->func = func;
	info->line = line;
	info->msg = msg;
	info->seq = alloc_count++;
	
	return info;
}

static void _inc_block_alloc_count(void *p, const char *func, const char *file, unsigned long line, const char *msg)
{
	rbtn_t *node = 0;
	block_info_t *info = 0;
	int added = 0;

	if (manipulating_block_tree)
		return;

	manipulating_block_tree = 1;
	node = rbtn_findins(ROOT(p), MEMCAST(p, int), 0, 1, 0, 1, 0, &added);

	if (added)
		info = node->extra = _add_new_block_info(p, func, file, line, msg);
	else
		info = (block_info_t*)node->extra;

	++info->alloc_count;

	manipulating_block_tree = 0;
}

static void _dec_block_alloc_count(void *p, const char *func, const char *file, unsigned long line, const char *msg)
{
	rbtn_t *node = 0;
	block_info_t *info = 0;
	int added = 0;

	if (manipulating_block_tree)
		return;

	manipulating_block_tree = 1;
	node = rbtn_findins(ROOT(p), MEMCAST(p, int), 0, 1, 0, 1, 0, &added);

	if (added)
		info = node->extra = _add_new_block_info(p, func, file, line, msg);
	else
		info = (block_info_t*)node->extra;

	--info->alloc_count;

	manipulating_block_tree = 0;
}

static void _log_blocks()
{
	block_info_t *info = first_block_info;
	unsigned int non_leaked_count = 0;

	while (info)
	{
		block_info_t *next = info->next;
		if (info->alloc_count != 0)
		{
			if (non_leaked_count)
				fprintf(log_file, "\n--- Successfully freed: %d blocks\n\n", non_leaked_count);

			fprintf(log_file, "%p\n\tSequence: %d, Allocation count: %d\n\tAllocated by: %s (%s : %d)\n\t%s\n", MEMCAST(info->p, void*), info->seq, info->alloc_count, info->func, info->file, info->line, info->msg ? info->msg : "[no message]");
			fflush(log_file);
			leaked_block_count++;

			non_leaked_count = 0;
		}
		else
			non_leaked_count++;
		
		free(info);
		info = next;
	}
	
	if (non_leaked_count)
		fprintf(log_file, "\n--- Successfully freed: %d blocks\n\n", non_leaked_count);
}

static void _finalize(void)
{
	int i;
	manipulating_block_tree = 1;

	leaked_block_count = 0;

	log_file = fopen("memlog.txt", "w");
	fprintf(log_file, "Leaked blocks:\n");
	_log_blocks();
	fprintf(log_file, "\nNumber of leaked blocks: %d out of %d allocations\n\n", leaked_block_count, alloc_count);
	fclose(log_file);

	for (i = 0; i < NUM_BLOCK_TREE_ROOTS; ++i)
		rbtn_free_all(block_tree_roots[i]);

	manipulating_block_tree = 0;
}

void *x_malloc(size_t size, const char *func, const char *file, unsigned long line, const char *msg)
{
	void *p;

	if (first_run)
	{
		atexit(_finalize);
		first_run = 0;
	}

	p = malloc(size);

	_inc_block_alloc_count(p, func, file, line, msg);

	return p;
}

void x_free(void *p, const char *func, const char *file, unsigned long line, const char *msg)
{
	if (first_run)
	{
		atexit(_finalize);
		first_run = 0;
	}
	free(p);

	_dec_block_alloc_count(p, func, file, line, msg);
}

