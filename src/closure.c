/* vim: set ts=4 sts=4 sw=4 noet ai: */
#include "global.h"

#include "gc.h"
#include "closure.h"

ref_t apply(ref_t func, ref_t args)
{
	closure_t *cls;
	ref_t result, param_frame;

	if (func.type != closure_type &&
		func.type != function_type &&
		func.type != macro_type)
	{
		LOG_ERROR("called with func not a closure type");
		return nil(); /* TODO: ERROR */
	}

	cls = (closure_t*)func.data.object;

	param_frame = make_stack(cls->env);

	/* register params in the params_frame */
	map_let(param_frame, cls->param_list, args);

	result = eval(cls->code, param_frame);

	release_ref(&param_frame);

	return result;
}

static void _closure_print_contents(closure_t *cls, FILE *to)
{
	assert(cls);

	print(cls->param_list, to);
	fprintf(to, " ");
	print(cls->code, to);
	fprintf(to, " ");
	print(cls->env, to);
}

static void closure_traits_gc_mark(ref_t instance)
{
	closure_t *cls = (closure_t*)instance.data.object;
	assert(cls);

	ref_gc_mark(cls->param_list);
	ref_gc_mark(cls->code);
	ref_gc_mark(cls->env);
}

static void closure_traits_gc_release_refs(ref_t instance)
{
	closure_t *cls = (closure_t*)instance.data.object;
	assert(cls);

	release_ref(&(cls->param_list));
	release_ref(&(cls->code));
	release_ref(&(cls->env));
}

static void closure_traits_gc_free_mem(ref_t instance)
{
	closure_t *cls = (closure_t*)instance.data.object;
	assert(cls);

	X_FREE(cls);
}

static void closure_traits_print(ref_t instance, FILE *to)
{
	fprintf(to, "#<closure ");
	_closure_print_contents((closure_t*)instance.data.object, to);
	fprintf(to, ">");
}

static ref_t closure_traits_type_name(ref_t instance)
{
	return make_symbol("closure", 0);
}

static ref_t closure_traits_execute(ref_t instance, ref_t args, ref_t calling_context)
{
	ref_t result, cl_args;
	trace(TRACE_FULL, "calling %r with args %r in env %r", instance, args, calling_context);
	cl_args = list(args, calling_context);
	result = apply(instance, cl_args);
	release_ref(&cl_args);
	return result;
}

static int closure_traits_eq(ref_t a, ref_t b)
{
	return a.data.object == b.data.object;
}

static int closure_traits_eql(ref_t a, ref_t b)
{
	closure_t *ac = (closure_t*)a.data.object;
	closure_t *bc = (closure_t*)b.data.object;

	return eql(ac->param_list, bc->param_list) & eql(ac->code, bc->code) & eql(ac->env, bc->env);
}

static const type_traits_t closure_traits =
{
	0, /* not evaluable */
	closure_traits_execute,
	closure_traits_print,
	closure_traits_type_name,
	closure_traits_eq,
	closure_traits_eql,
	gc_traits_addref,
	gc_traits_release,
	closure_traits_gc_mark,
	closure_traits_gc_release_refs,
	closure_traits_gc_free_mem
};
const type_traits_t *closure_type = &closure_traits;

/* function version */

static ref_t function_traits_execute(ref_t instance, ref_t args, ref_t calling_context)
{
	ref_t result, fn_args;
	fn_args = map_eval(args, calling_context);
	trace(TRACE_FULL, "calling %r with args %r", instance, fn_args);
	result = apply(instance, fn_args);
	release_ref(&fn_args);
	return result;
}

static void function_traits_print(ref_t instance, FILE *to)
{
	fprintf(to, "#<function ");
	_closure_print_contents((closure_t*)instance.data.object, to);
	fprintf(to, ">");
}

static ref_t function_traits_type_name(ref_t instance)
{
	return make_symbol("function", 0);
}

static const type_traits_t function_traits =
{
	0, /* not evaluable */
	function_traits_execute,
	function_traits_print,
	function_traits_type_name,
	closure_traits_eq,
	closure_traits_eql,
	gc_traits_addref,
	gc_traits_release,
	closure_traits_gc_mark,
	closure_traits_gc_release_refs,
	closure_traits_gc_free_mem
};
const type_traits_t *function_type = &function_traits;

/* macro version */

static ref_t macro_traits_execute(ref_t instance, ref_t args, ref_t calling_context)
{
	ref_t result, code;
	trace(TRACE_FULL, "calling %r with args %r", instance, args);
	code = apply(instance, args);
	result = eval(code, calling_context);
	release_ref(&code);
	return result;
}

static void macro_traits_print(ref_t instance, FILE *to)
{
	fprintf(to, "#<macro ");
	_closure_print_contents((closure_t*)instance.data.object, to);
	fprintf(to, ">");
}

static ref_t macro_traits_type_name(ref_t instance)
{
	return make_symbol("macro", 0);
}

static const type_traits_t macro_traits =
{
	0, /* not evaluable */
	macro_traits_execute,
	macro_traits_print,
	macro_traits_type_name,
	closure_traits_eq,
	closure_traits_eql,
	gc_traits_addref,
	gc_traits_release,
	closure_traits_gc_mark,
	closure_traits_gc_release_refs,
	closure_traits_gc_free_mem
};
const type_traits_t *macro_type = &macro_traits;

ref_t _make_closure(ref_t plist, ref_t code, ref_t env, const type_traits_t *vt)
{
	ref_t ref;
	closure_t *cls;

	if (env.type != stack_type)
	{
		LOG_ERROR("called with env not a stack frame.");
		return nil(); /* TODO: ERROR */
	}

	cls = (closure_t*)X_MALLOC(sizeof(closure_t));
	if (! cls)
	{
		/* TODO: ERROR */
		LOG_ERROR("out of memory error");
		return nil();
	}
	gc_init_object(&cls->gc, vt);

	cls->param_list = clone_ref(plist);
	cls->code = clone_ref(code);
	cls->env = clone_ref(env);

	ref.type = vt;
	ref.data.object = &cls->gc;
	return ref;
}

ref_t make_closure(ref_t plist, ref_t code, ref_t env)
{
	return _make_closure(plist, code, env, closure_type);
}

ref_t make_function(ref_t plist, ref_t code, ref_t env)
{
	return _make_closure(plist, code, env, function_type);
}

ref_t make_macro(ref_t plist, ref_t code, ref_t env)
{
	return _make_closure(plist, code, env, macro_type);
}
