#include "global.h"

#include "smalisp.h"

#include "gc.h"
#include "symbol.h"
#include "cons.h"
#include "closure.h"
#include "stack_frame.h"

static trace_level_t _sl_trace_level = TRACE_NONE;
static int _sl_trace_indent = 0;
static FILE *_sl_trace_file = 0;

trace_level_t get_trace_level()
{
	return _sl_trace_level;
}

void trace_inc_indent()
{
	if (_sl_trace_level > TRACE_NONE)
		++_sl_trace_indent;
}

void trace_dec_indent()
{
	if (_sl_trace_level > TRACE_NONE && _sl_trace_indent)
		--_sl_trace_indent;
}

void set_trace_file(FILE *fl)
{
	assert(fl);
	_sl_trace_file = fl;
}

void set_trace_level(trace_level_t new_level)
{
	if (_sl_trace_level == new_level)
		return;

	trace(TRACE_FULL, "Changing trace level");

	_sl_trace_level = new_level;
	
	if (_sl_trace_level && _sl_trace_file == 0)
		_sl_trace_file = stderr;

	trace(TRACE_FULL, "Trace level changed to FULL");
}

void trace(trace_level_t lvl, const char *fmt, ...)
{
	int i;
	const char *c;
	va_list args;

	if (! (_sl_trace_level >= lvl))
		return;

	assert(_sl_trace_file);

	if (_sl_trace_file == stderr) /* not a dedicated trace file, start items with TRACE: to identify them */
		fprintf(_sl_trace_file, "TRACE: ");
	
	for (i = 0; i != _sl_trace_indent; ++i)
		fputs("  ", _sl_trace_file);

	va_start(args, fmt);

	c = fmt;
	while (*c != 0)
	{
		if (*c == '%')
		{
			++c;

			switch (*c)
			{
			case 0:
				LOG_ERROR("trace called with an incomplete type specifier");
				return;
			case '%': fputc('%', _sl_trace_file); break;
			case 'i': fprintf(_sl_trace_file, "%i", va_arg(args, int)); break;
			case 'd': fprintf(_sl_trace_file, "%d", va_arg(args, int)); break;
			case 'f': fprintf(_sl_trace_file, "%f", va_arg(args, float)); break;
			case 'l':
				++c;
				if (*c == 'f')
					fprintf(_sl_trace_file, "%lf", va_arg(args, double));
				else
				{
					LOG_ERROR("trace called with invalid type specifier");
					return;
				}
				break;
			case 'L':
				++c;
				if (*c == 'i' || *c == 'd')
					fprintf(_sl_trace_file, "%Li", va_arg(args, __int64));
				else
				{
					LOG_ERROR("trace called with invalid type specifier");
					return;
				}
				break;
			case 'p': fprintf(_sl_trace_file, "%p", va_arg(args, void*)); break;
			case 'u': fprintf(_sl_trace_file, "%u", va_arg(args, unsigned int)); break;
			case 'x': fprintf(_sl_trace_file, "%x", va_arg(args, unsigned int)); break;
			case 'r': print(va_arg(args, ref_t), _sl_trace_file); break;
			default:
				LOG_ERROR("trace called with invalid type specifier");
				return;
			}
		}
		else
			fputc(*c, _sl_trace_file);

		++c;
	}

	va_end(args);

	fprintf(_sl_trace_file, "\n\n");
	fflush(_sl_trace_file);
}

static ref_t foreign_exec_traits_execute(ref_t instance, ref_t args, ref_t calling_context)
{
	return instance.data.fexec(args, calling_context);
}

static void foreign_exec_traits_print(ref_t instance, FILE *to)
{
	fprintf(to, "#<foreign-exec %p>", instance.data.fexec);
}

static ref_t foreign_exec_traits_type_name(ref_t instance)
{
	return make_symbol("foreign-exec", 0);
}

static int foreign_exec_traits_eq(ref_t a, ref_t b)
{
	return a.data.fexec == b.data.fexec;
}

static const type_traits_t foreign_exec_traits =
{
	0, /* not evaluable */
	foreign_exec_traits_execute,
	foreign_exec_traits_print,
	foreign_exec_traits_type_name,
	foreign_exec_traits_eq,
	foreign_exec_traits_eq, /* eq and eql do the same thing for foreign_execs */
	0, /* not ref counted */
	0,
	0, /* not garbage collected */
	0,
	0
};
const type_traits_t *foreign_exec_type = &foreign_exec_traits;

ref_t make_foreign_exec(foreign_exec_t func)
{
	ref_t ref;
	ref.type = foreign_exec_type;
	ref.data.fexec = func;
	return ref;
}

static void integer_traits_print(ref_t instance, FILE *to)
{
	fprintf(to, "%d", instance.data.integer);
}

static ref_t integer_traits_type_name(ref_t instance)
{
	return make_symbol("integer", 0);
}

static int integer_traits_eq(ref_t a, ref_t b)
{
	return a.data.integer == b.data.integer;
}

static const type_traits_t integer_traits =
{
	0, /* not evaluable */
	0, /* not executable */
	integer_traits_print,
	integer_traits_type_name,
	integer_traits_eq,
	integer_traits_eq, /* eq and eql do the same thing for integers */
	0, /* not ref counted */
	0,
	0, /* not garbage collected */
	0,
	0
};
const type_traits_t *integer_type = &integer_traits;

ref_t make_integer(int n)
{
	ref_t ref;
	ref.type = integer_type;
	ref.data.integer = n;
	return ref;
}

static void real_traits_print(ref_t instance, FILE *to)
{
	fprintf(to, "%lf", instance.data.real);
}

static ref_t real_traits_type_name(ref_t instance)
{
	return make_symbol("real", 0);
}

static int real_traits_eq(ref_t a, ref_t b)
{
	return a.data.real == b.data.real;
}

static const type_traits_t real_traits =
{
	0, /* not evaluable */
	0, /* not executable */
	real_traits_print,
	real_traits_type_name,
	real_traits_eq,
	real_traits_eq, /* eq and eql do the same thing for reals */
	0, /* not ref counted */
	0,
	0, /* not garbage collected */
	0,
	0
};
const type_traits_t *real_type = &real_traits;

ref_t make_real(double n)
{
	ref_t ref;
	ref.type = real_type;
	ref.data.real = n;
	return ref;
}

void print(ref_t val, FILE *to)
{
	if (val.type == NIL || val.type->print == 0)
		fprintf(to, "nil");
	else
		val.type->print(val, to);
}

void println(ref_t val, FILE *to)
{
	print(val, to);
	fprintf(to, "\n");
}
