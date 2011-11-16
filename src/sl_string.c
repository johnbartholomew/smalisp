#include "global.h"

#include "smalisp.h"
#include "gc.h"
#include "sl_string.h"

static unsigned long _hash(const char *s, size_t len)
{
	unsigned long ans = 0;
	char *a = (char*)&ans;
	size_t i;
	
	for (i = 0; i < len; i++)
		a[i % sizeof(unsigned long)] ^= *s++;
	
	return ans;
}

static void _print_escaped(string_t *str, FILE *to)
{
	char *s = (char*)str + sizeof(string_t);
	char *end = s + str->len;
	char *c;
	
	fprintf(to, "\"");
	for (c = s; c != end; ++c)
	{
		if (*c == '\\')
			fprintf(to, "\\\\");
		else if (!isgraph(*c) && *c != ' ')
		{
			switch (*c)
			{
			case '\n': fprintf(to, "\\n"); break;
			case '\r': fprintf(to, "\\r"); break;
			case '\b': fprintf(to, "\\b"); break;
			case '\t': fprintf(to, "\\t"); break;
			case '\\': fprintf(to, "\\\\"); break;
			case 0: fprintf(to, "\\0"); break;
			default:
				fprintf(to, "\\%d", (int)*c);
				break;
			}
		}
		else
			fprintf(to, "%c", *c);
	}
	fprintf(to, "\"");
}

static void string_traits_addref(ref_t instance)
{
	string_t *str = instance.data.str;
	assert(str);
	++str->rc;
}

static void string_traits_release(ref_t instance)
{
	string_t *str = instance.data.str;
	assert(str);
	if (!--str->rc)
		X_FREE(str);
}

static void string_traits_print(ref_t instance, FILE *to)
{
	string_t *str = instance.data.str;
	assert(str);
	_print_escaped(str, to);
}

static ref_t string_traits_type_name(ref_t instance)
{
	return make_symbol("string", 0);
}

static int string_traits_eq(ref_t a, ref_t b)
{
	return a.data.str == b.data.str;
}

static int string_traits_eql(ref_t a, ref_t b)
{
	char *s1, *s2;
	size_t len1, len2;
	len1 = a.data.str->len;
	len2 = b.data.str->len;
	s1 = (char*)a.data.str + sizeof(string_t);
	s2 = (char*)b.data.str + sizeof(string_t);
	return (len1 == len2) && (memcmp(s1, s2, len1) == 0);
}

#if 0
static ref_t string_traits_eval(ref_t instance, ref_t context)
{
	ref_t result;
	trace(TRACE_FULL, "evaluating symbol \"%r\"", instance);
	result = stack_assoc(context, instance);
	trace(TRACE_FULL, "evaluated to: %r", result);
	return result;
}
#endif

static const type_traits_t string_traits =
{
	0, /* not evaluable */
	0, /* not executable */
	string_traits_print,
	string_traits_type_name,
	string_traits_eq,
	string_traits_eql,
	string_traits_addref,
	string_traits_release,
	0, /* not garbage collected */
	0,
	0
};
const type_traits_t *string_type = &string_traits;

ref_t make_string(const char *s, size_t len)
{
	unsigned long hash;
	string_t *str = 0;
	char *str_s;
	ref_t ref;
	
	if (len == 0)
		len = strlen(s);

	hash = _hash(s, len);
	str = (string_t*)XX_MALLOC(sizeof(string_t) + len + 1, s);
	str->rc = 1;
	str->hash = hash;
	str->len = len;
	str_s = (char*)str + sizeof(string_t);
	memcpy(str_s, s, len);
	str_s[len] = 0;

	ref.type = string_type;
	ref.data.str = str;
	return ref;
}

const char *string_c_str(string_t *str)
{
	return (const char*)str + sizeof(string_t);
}
