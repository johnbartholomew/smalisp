/* vim: set ts=4 sts=4 sw=4 noet ai: */
#include "global.h"

#include "rbt.h"

#include "smalisp.h"
#include "symbol.h"

#define NUM_SYMBOL_TREES (16)
static rbtn_t *symbol_trees[NUM_SYMBOL_TREES] = {0}; /* set of red-black tree root nodes */

int symbol_eval_count = 0;

typedef struct binding_ts binding_t;
struct binding_ts
{
	ref_t val;
	size_t stack_pos;
};

static int _symbol_rbt_cmp(void *s1, void *s2)
{
	int a, b;
	size_t i = 0;

	const char *src = string_c_str((string_t*)s1),
		       *dst = string_c_str(((symbol_t*)s2)->name);
	size_t len_dst = ((symbol_t*)s2)->name->len;
	size_t len_src = ((string_t*)s1)->len;

    do {
		a = (i < len_dst) ? *dst++ : 0;
		b = (i < len_src) ? *src++ : 0;
		i++;
    } while (a && (a == b));

    return (a-b);
}

static int _is_safe(symbol_t *symb)
{
	const char *str = string_c_str(symb->name);
	const char *end = str + symb->name->len;

	if ((*str < 'a' || *str > 'z') &&
		(*str < 'A' || *str > 'Z') &&
		(strchr("_-+*/%^$!&=<>?~@:;", *str) == 0))
		return 0;

	++str;
	for (; str != end; ++str)
	{
		if ((*str < 'a' || *str > 'z') &&
			(*str < 'A' || *str > 'Z') &&
			(*str < '0' || *str > '9') &&
			(strchr("_-+*/%^$!&=<>?~@:;", *str) == 0))
		{
			return 0;
		}
	}

	return 1;
}

static void _print_escaped(symbol_t *symb, FILE *to)
{
	const char *str = string_c_str(symb->name);
	const char *end = str + symb->name->len;
	const char *c;
	
	fprintf(to, "|");
	for (c = str; c != end; ++c)
	{
		if (!isgraph(*c) && *c != ' ')
			fprintf(to, "\\%d", (int)*c);
		else if (*c == '|' || *c == '\\')
			fprintf(to, "\\%c", *c);
		else
			fprintf(to, "%c", *c);
	}
	fprintf(to, "|");
}

static void _make_safe(symbol_t *symb, char *buf, size_t max_len)
{
	const char *str = string_c_str(symb->name);
	const char *end = str + symb->name->len;
	const char *c;

	assert(max_len > 16); /* if it's less than that, then it's pretty damn pointless */

	for (c = str; max_len && c != end; --max_len, ++c)
	{
		if (*c < ' ' || *c > 127 || *c == '\\')
		{
			switch (*c)
			{
			case '\n': *buf++ = '\\'; *buf++ = 'n'; break;
			case '\r': *buf++ = '\\'; *buf++ = 'r'; break;
			case '\t': *buf++ = '\\'; *buf++ = 't'; break;
			case '\b': *buf++ = '\\'; *buf++ = 'b'; break;
			case '\0': *buf++ = '\\'; *buf++ = '0'; break;
			case '\\': *buf++ = '\\'; *buf++ = '\\'; break;
			default: *buf++ = '\\'; *buf++ = '?'; break;
			}
		}
		else
			*buf++ = *c;
	}

	if (c < end)
	{
		buf[max_len - 4] = '.';
		buf[max_len - 3] = '.';
		buf[max_len - 2] = '.';
	}
	buf[max_len - 1] = 0;
}

static void symbol_traits_addref(ref_t instance)
{
	symbol_t *symb = instance.data.symb;
	assert(symb);
	++symb->rc;
}

static void symbol_traits_release(ref_t instance)
{
	symbol_t *symb = instance.data.symb;
	assert(symb);
	if (! --symb->rc)
	{
		ref_t ref;
		rbtn_t **root = &symbol_trees[symb->name->hash % NUM_SYMBOL_TREES];
		rbtn_del(root, 0, symb->name, _symbol_rbt_cmp, 0, 0);

		assert(string_type && string_type->release);
		ref.type = string_type;
		ref.data.str = symb->name;

		/* do not need to release the references in the binding stack,
		   because they are weak refs */
		vector_clear(&symb->binding_stack);
		XX_FREE(symb, string_c_str(symb->name));

		string_type->release(ref);
	}
}

static void symbol_traits_print(ref_t instance, FILE *to)
{
	symbol_t *symb = instance.data.symb;
	assert(symb);
	if (_is_safe(symb))
		fprintf(to, "%s", string_c_str(symb->name));
	else
		_print_escaped(symb, to);
}

static ref_t symbol_traits_type_name(ref_t instance)
{
	return make_symbol("symbol", 0);
}

static int symbol_traits_eq(ref_t a, ref_t b)
{
	return a.data.symb == b.data.symb;
}

static ref_t symbol_traits_eval(ref_t instance, ref_t context)
{
	symbol_t *symb;
	ref_t result;

	symb = instance.data.symb;
	trace(TRACE_FULL, "evaluating symbol \"%r\"", instance);

	if (symb->binding_stack.size == 0)
	{
		LOG_ERROR_X("Unbound symbol: %s", string_c_str(symb->name));
		result = nil();
	}
	else
	{
		binding_t *it = (binding_t*)vector_back(&symb->binding_stack);
		result = clone_ref(it->val);
	}

	++symbol_eval_count;

	trace(TRACE_FULL, "evaluated to: %r", result);
	return result;
}

static const type_traits_t symbol_traits =
{
	symbol_traits_eval,
	0, /* not executable */
	symbol_traits_print,
	symbol_traits_type_name,
	symbol_traits_eq,
	symbol_traits_eq, /* eq and eql do the same thing for symbols */
	symbol_traits_addref,
	symbol_traits_release,
	0, /* not garbage collected */
	0,
	0
};

const type_traits_t *symbol_type = &symbol_traits;

ref_t make_symbol(const char *name, size_t len)
{
	symbol_t *symb = 0;
	string_t *str = 0;
	int added = 0;
	rbtn_t **root, *node = 0;
	ref_t name_ref, ref;

	name_ref = make_string(name, len);

	str = name_ref.data.str;

	root = &symbol_trees[str->hash % NUM_SYMBOL_TREES];

	node = rbtn_findins(root, 0, str, 1, _symbol_rbt_cmp, 0, 0, &added);
	if (added)
	{
		symb = (symbol_t*)XX_MALLOC(sizeof(symbol_t), string_c_str(str));
		add_ref(name_ref);
		symb->name = str;
		symb->rc = 1;
		VECTOR_INIT_TYPE(&symb->binding_stack, binding_t);

		node->data = symb;
	}
	else
	{
		symb = (symbol_t*)node->data;
		++symb->rc;
	}

	release_ref(&name_ref);
	ref.type = symbol_type;
	ref.data.symb = symb;
	return ref;
}

const char *symbol_c_str(ref_t ref)
{
	static char safe_buf[1024];
	symbol_t *symb;
	size_t len;

	if (ref.type != symbol_type)
	{
		LOG_ERROR("called with a non-symbol type");
		return 0;
	}

	symb = ref.data.symb;
	assert(symb && symb->name);
	len = symb->name->len;

	if (_is_safe(symb) && len < 1024)
	{
		memcpy(safe_buf, string_c_str(symb->name), len);
		safe_buf[len] = 0;
	}
	else
		_make_safe(symb, safe_buf, 1024);
	
	return safe_buf;
}

static binding_t *find_binding(symbol_t *symb, size_t start_frame)
{
	binding_t *it, *front;

	front = (binding_t*)symb->binding_stack.items;
	it = (binding_t*)vector_end(&symb->binding_stack);
	while (it != front)
	{
		--it;

		if (it->stack_pos <= start_frame)
			return it;
	}

	return 0;
}

void symbol_let(ref_t symbol, ref_t value, size_t frame)
{
	symbol_t *symb = symbol.data.symb;
	binding_t *cur_val;

	if (symbol.type != symbol_type)
	{
		LOG_ERROR("Called with a non-symbol.");
		return;
	}

	cur_val = find_binding(symb, frame);
	if (!cur_val)
	{
		cur_val = vector_insert(&symb->binding_stack, VECTOR_NPOS);
		cur_val->stack_pos = frame;
		cur_val->val = value;
		return;
	}
	else
	{
		if (cur_val->stack_pos == frame)
			cur_val->val = value;
		else if (cur_val->stack_pos < frame)
		{
			size_t n = vector_idx_from_it(&symb->binding_stack, cur_val);
			cur_val = (binding_t*)vector_insert(&symb->binding_stack, n + 1);

			cur_val->stack_pos = frame;
			cur_val->val = value;
		}
		else
		{
			LOG_ERROR("WTFLOL THIS CAN'T HAPPEN!");
		}
	}
}

void symbol_set(ref_t symbol, ref_t new_value, size_t start_frame)
{
	symbol_t *symb = symbol.data.symb;
	binding_t *cur_val;

	if (symbol.type != symbol_type)
	{
		LOG_ERROR("Called with a non-symbol.");
		return;
	}

	cur_val = find_binding(symb, start_frame);
	if (!cur_val)
	{
		LOG_ERROR("Attempting to rebind and unbound symbol.");
		return;
	}
	else
		cur_val->val = new_value;
}

void symbol_unset(ref_t symbol, size_t frame)
{
	symbol_t *symb = symbol.data.symb;
	binding_t *cur_val;
	size_t n;

	if (symbol.type != symbol_type)
	{
		LOG_ERROR("Called with a non-symbol.");
		return;
	}

	cur_val = find_binding(symb, frame);
	if (!cur_val || cur_val->stack_pos != frame)
		return;

	n = vector_idx_from_it(&symb->binding_stack, cur_val);
	vector_erase(&symb->binding_stack, n);
}
