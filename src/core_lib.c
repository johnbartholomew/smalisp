/* vim: set ts=4 sts=4 sw=4 noet ai: */
#include "global.h"

#include "smalisp.h"
#include "stack_frame.h"
#include "closure.h"

#include "core_lib.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

static FILE *_sl_print_fl = 0;
static FILE *_sl_read_fl = 0;

void set_print_file(FILE *fl)
{
	_sl_print_fl = fl;
}

void set_read_file(FILE *fl)
{
	_sl_read_fl = fl;
}

ref_t slfe_quote(ref_t args, ref_t assoc)
{
	return car(args);
}

ref_t slfe_eq(ref_t args, ref_t assoc)
{
	ref_t result;
	ref_t processed_args = map_eval(args, assoc);
	ref_t a = car(processed_args);
	ref_t b = cadr(processed_args);
	release_ref(&processed_args);

	if (eq(a, b))
		result = make_symbol("t", 0);
	else
		result = nil();

	release_ref(&a);
	release_ref(&b);

	return result;
}

ref_t slfe_eql(ref_t args, ref_t assoc)
{
	ref_t result;
	ref_t processed_args = map_eval(args, assoc);
	ref_t a = car(processed_args);
	ref_t b = cadr(processed_args);
	release_ref(&processed_args);

	if (eql(a, b))
		result = make_symbol("t", 0);
	else
		result = nil();

	release_ref(&a);
	release_ref(&b);

	return result;
}

ref_t slfe_cond(ref_t args, ref_t assoc)
{
	ref_t result;
	ref_t test, test_result, body, rest;

	if (args.type == NIL)
		return nil();

	test = caar(args);
	test_result = eval(test, assoc);
	release_ref(&test);

	if (test_result.type != NIL)
	{
		body = cadar(args);
		result = eval(body, assoc);
		release_ref(&body);
	}
	else
	{
		rest = cdr(args);
		result = slfe_cond(rest, assoc);
		release_ref(&rest);
	}
	release_ref(&test_result);
	return result;
}

ref_t slfe_car(ref_t args, ref_t assoc)
{
	ref_t result, arg, arge;
	arg = car(args);
	arge = eval(arg, assoc);
	release_ref(&arg);
	result = car(arge);
	release_ref(&arge);
	return result;
}

ref_t slfe_cdr(ref_t args, ref_t assoc)
{
	ref_t result, arg, arge;
	arg = car(args);
	arge = eval(arg, assoc);
	release_ref(&arg);
	result = cdr(arge);
	release_ref(&arge);
	return result;
}

ref_t slfe_atom(ref_t args, ref_t assoc)
{
	ref_t result, arg, arge;
	arg = car(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	if (arge.type == cons_type)
		result = nil();
	else
		result = make_symbol("t", 0);

	release_ref(&arge);
	return result;
}

ref_t slfe_macro(ref_t args, ref_t assoc)
{
	ref_t result, param_list, code;

	param_list = car(args);
	code = cadr(args);
	result = make_macro(param_list, code, assoc);
	release_ref(&param_list);
	release_ref(&code);

	return result;
}

ref_t slfe_fn(ref_t args, ref_t assoc)
{
	ref_t result, param_list, code;

	param_list = car(args);
	code = cadr(args);
	result = make_function(param_list, code, assoc);
	release_ref(&param_list);
	release_ref(&code);

	return result;
}

ref_t slfe_closure(ref_t args, ref_t assoc)
{
	ref_t result, param_list, code;

	param_list = car(args);
	code = cadr(args);
	result = make_closure(param_list, code, assoc);
	release_ref(&param_list);
	release_ref(&code);

	return result;
}

ref_t slfe_set(ref_t args, ref_t assoc)
{
	ref_t name, arg, arge;

	name = car(args);
	arg = cadr(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	trace(TRACE_FULL, "Setting %r to %r in stack %r", name, arge, assoc);

	stack_set(assoc, name, arge);
	release_ref(&name);

	return arge;
}

ref_t slfe_env_set(ref_t args, ref_t assoc)
{
	ref_t name, namee, arg, arge, env, enve;

	name = car(args);
	namee = eval(name, assoc);
	release_ref(&name);

	arg = cadr(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	env = caddr(args);
	enve = eval(env, assoc);
	release_ref(&env);

	stack_set(enve, namee, arge);

	release_ref(&namee);
	release_ref(&enve);

	return arge;
}

ref_t slfe_let(ref_t args, ref_t assoc)
{
	ref_t name, arg, arge;

	name = car(args);
	arg = cadr(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	trace(TRACE_FULL, "Registerng %r as %r in stack %r", name, arge, assoc);

	stack_let(assoc, name, arge);
	release_ref(&name);

	return arge;
}

ref_t slfe_env_let(ref_t args, ref_t assoc)
{
	ref_t name, namee, arg, arge, env, enve;

	name = car(args);
	namee = eval(name, assoc);
	release_ref(&name);

	arg = cadr(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	env = caddr(args);
	enve = eval(env, assoc);
	release_ref(&env);

	stack_let(enve, namee, arge);

	release_ref(&namee);
	release_ref(&enve);

	return arge;
}

ref_t slfe_cons(ref_t args, ref_t assoc)
{
	ref_t result, arga, argae, argb, argbe;

	arga = car(args);
	argae = eval(arga, assoc);
	release_ref(&arga);

	argb = cadr(args);
	argbe = eval(argb, assoc);
	release_ref(&argb);

	result = make_cons(argae, argbe);
	release_ref(&argae);
	release_ref(&argbe);

	return result;
}

ref_t slfe_do(ref_t args, ref_t assoc)
{
	ref_t result, first, firste, rest;

	first = car(args);
	firste = eval(first, assoc);
	release_ref(&first);

	rest = cdr(args);
	if (rest.type == NIL)
	{
		release_ref(&rest);
		return firste;
	}
	else
	{
		release_ref(&firste);
		result = slfe_do(rest, assoc);
		release_ref(&rest);
		return result;
	}
}

ref_t slfe_scope(ref_t args, ref_t assoc)
{
	ref_t result, env;

	env = make_stack(assoc);
	result = slfe_do(args, env);
	release_ref(&env);
	
	return result;
}

ref_t slfe_apply(ref_t args, ref_t assoc)
{
	ref_t result, fn, fne, arglist, argliste;

	fn = car(args);
	fne = eval(fn, assoc);
	release_ref(&fn);

	arglist = cadr(args);
	argliste = eval(arglist, assoc);
	release_ref(&arglist);

	result = call(fne, argliste, assoc);
	release_ref(&fne);
	release_ref(&argliste);

	return result;
}

ref_t slfe_macro_expand(ref_t args, ref_t assoc)
{
	ref_t result, macro, macroe, arglist;

	macro = car(args);
	macroe = eval(macro, assoc);
	release_ref(&macro);

	arglist = cdr(args);
	result = apply(macroe, arglist);
	release_ref(&macroe);
	release_ref(&arglist);

	return result;
}

ref_t slfe_closure_code(ref_t args, ref_t assoc)
{
	ref_t result, fn, fne;
	closure_t *cls;

	fn = car(args);
	fne = eval(fn, assoc);
	release_ref(&fn);

	cls = (closure_t*)fne.data.object;
	result = clone_ref(cls->code);
	release_ref(&fne);

	return result;
}

ref_t slfe_closure_env(ref_t args, ref_t assoc)
{
	ref_t result, fn, fne;
	closure_t *cls;

	fn = car(args);
	fne = eval(fn, assoc);
	release_ref(&fn);

	cls = (closure_t*)fne.data.object;
	result = clone_ref(cls->env);
	release_ref(&fne);

	return result;
}

ref_t slfe_closure_plist(ref_t args, ref_t assoc)
{
	ref_t result, fn, fne;
	closure_t *cls;

	fn = car(args);
	fne = eval(fn, assoc);
	release_ref(&fn);

	cls = (closure_t*)fne.data.object;
	result = clone_ref(cls->param_list);
	release_ref(&fne);

	return result;
}

ref_t slfe_make_closure(ref_t args, ref_t assoc)
{
	ref_t result, plist, pliste, code, codee, env, enve;

	plist = car(args);
	pliste = eval(plist, assoc);
	release_ref(&plist);

	code = cadr(args);
	codee = eval(code, assoc);
	release_ref(&code);

	env = caddr(args);
	enve = eval(env, assoc);
	release_ref(&env);

	result = make_closure(pliste, codee, enve);
	release_ref(&pliste);
	release_ref(&codee);
	release_ref(&enve);

	return result;
}

ref_t slfe_print(ref_t args, ref_t assoc)
{
	ref_t arg, arge;
	arg = car(args);
	arge = eval(arg, assoc);
	release_ref(&arg);
	println(arge, _sl_print_fl ? _sl_print_fl : stdout);
	return arge;
}

ref_t slfe_read(ref_t args, ref_t assoc)
{
	return read(_sl_read_fl ? _sl_read_fl : stdin);
}

ref_t slfe_eval(ref_t args, ref_t assoc)
{
	ref_t result, arg, arge, env, enve;
	arg = car(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	env = cadr(args);
	if (env.type != NIL)
	{
		enve = eval(env, assoc);
		result = eval(arge, enve);
		release_ref(&enve);
	}
	else
		result = eval(arge, assoc);

	release_ref(&env);
	release_ref(&arge);
	return result;
}

ref_t slfe_get_env(ref_t args, ref_t assoc)
{
	return clone_ref(assoc);
}

ref_t slfe_type(ref_t args, ref_t assoc)
{
	ref_t result, arg, arge;

	arg = car(args);
	arge = eval(arg, assoc);
	release_ref(&arg);

	if (arge.type == NIL || arge.type->type_name == 0)
		result = nil();
	else
		result = arge.type->type_name(arge);
	release_ref(&arge);

	return result;
}

ref_t slfe_add(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if (ae.type != be.type)
		result = nil();
	else
	{
		if (ae.type == integer_type)
			result = make_integer(ae.data.integer + be.data.integer);
		else if (ae.type == real_type)
			result = make_real(ae.data.real + be.data.real);
		else
			result = nil();
	}

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_sub(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if (ae.type != be.type)
		result = nil();
	else
	{
		if (ae.type == integer_type)
			result = make_integer(ae.data.integer - be.data.integer);
		else if (ae.type == real_type)
			result = make_real(ae.data.real - be.data.real);
		else
			result = nil();
	}

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_mul(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if (ae.type != be.type)
		result = nil();
	else
	{
		if (ae.type == integer_type)
			result = make_integer(ae.data.integer * be.data.integer);
		else if (ae.type == real_type)
			result = make_real(ae.data.real * be.data.real);
		else
			result = nil();
	}

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_div(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if (ae.type != be.type)
		result = nil();
	else
	{
		if (ae.type == integer_type)
			result = make_integer(ae.data.integer / be.data.integer);
		else if (ae.type == real_type)
			result = make_real(ae.data.real / be.data.real);
		else
			result = nil();
	}

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_mod(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if ((ae.type != be.type) || ae.type != integer_type)
		result = nil();
	else
		result = make_integer(ae.data.integer % be.data.integer);

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_bitand(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if ((ae.type != be.type) || ae.type != integer_type)
		result = nil();
	else
		result = make_integer(ae.data.integer & be.data.integer);

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_bitor(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if ((ae.type != be.type) || ae.type != integer_type)
		result = nil();
	else
		result = make_integer(ae.data.integer | be.data.integer);

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_bitxor(ref_t args, ref_t assoc)
{
	ref_t a, ae, b, be, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);
	b = cadr(args);
	be = eval(b, assoc);
	release_ref(&b);

	if ((ae.type != be.type) || ae.type != integer_type)
		result = nil();
	else
		result = make_integer(ae.data.integer ^ be.data.integer);

	release_ref(&ae);
	release_ref(&be);
	return result;
}

ref_t slfe_bitnot(ref_t args, ref_t assoc)
{
	ref_t a, ae, result;
	a = car(args);
	ae = eval(a, assoc);
	release_ref(&a);

	if (ae.type != integer_type)
		result = nil();
	else
		result = make_integer(~ ae.data.integer);

	release_ref(&ae);
	return result;
}

ref_t slfe_gc_collect(ref_t args, ref_t assoc)
{
	collect_garbage();
	return nil();
}

static ref_t _do_quasiquote(ref_t v, ref_t e)
{
	ref_t result, unquote;
	unquote = make_symbol("unquote", 0);

	if (v.type == cons_type)
	{
		ref_t lar;
		lar = car(v);
		if (eql(lar, unquote))
		{
			ref_t ladr = cadr(v);
			result = eval(ladr, e);
			release_ref(&ladr);
		}
		else
		{
			ref_t a, b, ldr;
			a = _do_quasiquote(lar, e);
			ldr = cdr(v);
			b = _do_quasiquote(ldr, e);
			release_ref(&ldr);
			result = make_cons(a, b);
			release_ref(&a);
			release_ref(&b);
		}
		release_ref(&lar);
	}
	else
		result = clone_ref(v);

	release_ref(&unquote);
	return result;
}

ref_t slfe_quasiquote(ref_t args, ref_t assoc)
{
	ref_t result, arg;
	arg = car(args);
	result = _do_quasiquote(arg, assoc);
	release_ref(&arg);
	return result;
}

void register_core_lib(ref_t env)
{
	REG_FN(quote, env);
	REG_FN(eq, env);
	REG_FN(eql, env);
	REG_FN(cond, env);
	REG_FN(do, env);
	REG_FN(scope, env);
	REG_FN(apply, env);
	REG_FN(car, env);
	REG_FN(cdr, env);
	REG_FN(cons, env);
	REG_FN(atom, env);
	REG_FN(closure, env);
	REG_FN(macro, env);
	REG_FN(fn, env);
	REG_FN(set, env);
	REG_FN(let, env);
	REG_FN(read, env);
	REG_FN(eval, env);
	REG_FN(print, env);
	REG_FN(type, env);
	REG_FN(quasiquote, env);
	REG_NAMED_FN("macro-expand", slfe_macro_expand, env);
	REG_NAMED_FN("get-env", slfe_get_env, env);
	REG_NAMED_FN("env-set", slfe_env_set, env);
	REG_NAMED_FN("env-let", slfe_env_let, env);
	REG_NAMED_FN("gc-collect", slfe_gc_collect, env);

	REG_NAMED_FN("closure-code", slfe_closure_code, env);
	REG_NAMED_FN("closure-param-list", slfe_closure_plist, env);
	REG_NAMED_FN("closure-env", slfe_closure_env, env);
	REG_NAMED_FN("make-closure", slfe_make_closure, env);

	REG_NAMED_FN("+", slfe_add, env);
	REG_NAMED_FN("-", slfe_sub, env);
	REG_NAMED_FN("*", slfe_mul, env);
	REG_NAMED_FN("/", slfe_div, env);
	REG_NAMED_FN("%", slfe_mod, env);
	
	REG_NAMED_FN("&", slfe_bitand, env);
	REG_NAMED_FN("|", slfe_bitor, env);
	REG_NAMED_FN("^", slfe_bitxor, env);
	REG_NAMED_FN("~", slfe_bitnot, env);
}
