#include "global.h"

#include "smalisp.h"
#include "gc.h"
#include "cons.h"

static void cons_traits_gc_mark(ref_t instance)
{
	cons_t *cons = (cons_t*)instance.data.object;
	assert(cons);
	ref_gc_mark(cons->car);
	ref_gc_mark(cons->cdr);
}

static void cons_traits_gc_release_refs(ref_t instance)
{
	cons_t *cons = (cons_t*)instance.data.object;
	assert(cons);
	release_ref(&cons->car);
	release_ref(&cons->cdr);
}

static void cons_traits_gc_free_mem(ref_t instance)
{
	cons_t *cons = (cons_t*)instance.data.object;
	assert(cons);
	X_FREE(cons);
}

static void cons_traits_print(ref_t instance, FILE *to)
{
	cons_t *cons = (cons_t*)instance.data.object;
	assert(cons);

	fprintf(to, "(");

	while (cons)
	{
		print(cons->car, to);

		if (cons->cdr.type == NIL)
			cons = 0;
		else if (cons->cdr.type == cons_type)
		{
			fprintf(to, " ");
			cons = (cons_t*)cons->cdr.data.object;
		}
		else
		{
			fprintf(to, " . ");
			print(cons->cdr, to);
			cons = 0;
		}
	}

	fprintf(to, ")");
}

static ref_t cons_traits_type_name(ref_t instance)
{
	return make_symbol("cons", 0);
}

static int cons_traits_eq(ref_t a, ref_t b)
{
	return a.data.object == b.data.object;
}

static int cons_traits_eql(ref_t a, ref_t b)
{
	cons_t *ac = (cons_t*)a.data.object;
	cons_t *bc = (cons_t*)b.data.object;
	return eql(ac->car, bc->car) && eql(ac->cdr, bc->cdr);
}

static ref_t cons_traits_eval(ref_t instance, ref_t context)
{
	ref_t result;
	ref_t lar;

	trace(TRACE_FULL, "evaluating cons: %r", instance);

	lar = car(instance);

	if (!lar.type)
	{
		LOG_ERROR("trying to evaluate a cons with a nil car");
		result = nil();
	}
	else if (!lar.type->execute && !lar.type->eval)
	{
		LOG_ERROR("trying to evaluate a cons with a non-executable, non-evaluable car");
		result = nil();
	}
	else if (lar.type->execute)
	{
		ref_t args = cdr(instance);
		result = lar.type->execute(lar, args, context);
		release_ref(&args);
	}
	else if (lar.type->eval)
	{
		/* evaluate the car in case it can be turned into a callable,
		   and then try to re-evaluate the cons */
		ref_t new_lar, ldr, new_cons;

		new_lar = eval(lar, context);
		ldr = cdr(instance);

		new_cons = make_cons(new_lar, ldr);
		release_ref(&new_lar);
		release_ref(&ldr);

		result = eval(new_cons, context);
		release_ref(&new_cons);
	}

	release_ref(&lar);
	trace(TRACE_FULL, "evaluated to: %r", result);
	return result;
}

static const type_traits_t cons_traits =
{
	cons_traits_eval,
	0, /* not executable */
	cons_traits_print,
	cons_traits_type_name,
	cons_traits_eq,
	cons_traits_eql,
	gc_traits_addref,
	gc_traits_release,
	cons_traits_gc_mark,
	cons_traits_gc_release_refs,
	cons_traits_gc_free_mem
};
const type_traits_t *cons_type = &cons_traits;

ref_t make_cons(ref_t car, ref_t cdr)
{
	ref_t ref;
	cons_t *cons;

	if (car.type == NIL && cdr.type == NIL)
		return nil();

	cons = (cons_t*)X_MALLOC(sizeof(cons_t));
	gc_init_object(&cons->gc, cons_type);

	cons->car = car;
	add_ref(car);
	cons->cdr = cdr;
	add_ref(cdr);

	ref.type = cons_type;
	ref.data.object = &cons->gc;
	return ref;
}
