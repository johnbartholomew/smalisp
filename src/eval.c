#include "global.h"

#include "smalisp.h"

#include "cons.h"
#include "closure.h"

int eql(ref_t a, ref_t b)
{
	if (a.type != b.type)
		return 0;

	if (a.type == NIL)
		return -1;

	if (!a.type->eql)
		return 0;

	return a.type->eql(a, b);
}

int eq(ref_t a, ref_t b)
{
	if (a.type != b.type)
		return 0;

	if (a.type == NIL)
		return -1;

	if (!a.type->eql)
		return 0;

	return a.type->eq(a, b);
}

ref_t car(ref_t val)
{
	cons_t *cons;

	if (val.type != cons_type)
		return nil();

	cons = (cons_t*)val.data.object;
	return clone_ref(cons->car);
}

ref_t cdr(ref_t val)
{
	cons_t *cons;

	if (val.type != cons_type)
		return nil();

	cons = (cons_t*)val.data.object;
	return clone_ref(cons->cdr);
}

ref_t cadr(ref_t l)
{
	ref_t ldr = cdr(l);
	ref_t ladr = car(ldr);
	release_ref(&ldr);
	return ladr;
}

ref_t caddr(ref_t l)
{
	ref_t ldr = cdr(l);
	ref_t lddr = cdr(ldr);
	ref_t laddr;
	release_ref(&ldr);
	laddr = car(lddr);
	release_ref(&lddr);
	return laddr;
}

ref_t caar(ref_t l)
{
	ref_t lar = car(l);
	ref_t laar = car(lar);
	release_ref(&lar);
	return laar;
}

ref_t cadar(ref_t l)
{
	ref_t lar = car(l);
	ref_t ladar = cadr(lar);
	release_ref(&lar);
	return ladar;
}

ref_t list(ref_t a, ref_t b)
{
	ref_t cons, ldr;

	ldr = make_cons(b, nil());
	cons = make_cons(a, ldr);
	release_ref(&ldr);

	return cons;
}

ref_t list3(ref_t a, ref_t b, ref_t c)
{
	ref_t cons, ldr;

	ldr = list(b, c);
	cons = make_cons(a, ldr);
	release_ref(&ldr);

	return cons;
}

ref_t list_from_array(ref_t *refs, size_t num)
{
	ref_t cons, ldr;
	if (num > 1)
		ldr = list_from_array(refs + 1, num - 1);
	else
		ldr = nil();
	cons = make_cons(refs[0], ldr);
	release_ref(&ldr);

	return cons;
}

void map_let(ref_t frame, ref_t names, ref_t vals)
{
	ref_t nar, var, ndr, vdr;

	if (frame.type != stack_type)
	{
		LOG_ERROR("LOLOL");
		return;
	}

	if (names.type == NIL)
		return;

	nar = car(names);
	var = car(vals);
	stack_let(frame, nar, var);
	release_ref(&nar);
	release_ref(&var);

	ndr = cdr(names);
	vdr = cdr(vals);
	map_let(frame, ndr, vdr);
	release_ref(&ndr);
	release_ref(&vdr);
}

ref_t call(ref_t exec, ref_t args, ref_t assoc)
{
	ref_t result;

	trace_inc_indent();

	if (exec.type == NIL || exec.type->execute == 0)
	{
		LOG_ERROR("called with nil exec");
		result = nil();
	}
	else
		result = exec.type->execute(exec, args, assoc);

	trace_dec_indent();

	return result;
}

ref_t map_eval(ref_t l, ref_t assoc)
{
	ref_t lar, ldr, elar, eldr, cons;

	if (l.type == NIL)
		return nil();

	lar = car(l);
	elar = eval(lar, assoc);
	release_ref(&lar);

	ldr = cdr(l);
	eldr = map_eval(ldr, assoc);
	release_ref(&ldr);

	cons = make_cons(elar, eldr);
	release_ref(&elar);
	release_ref(&eldr);

	return cons;
}

ref_t eval(ref_t e, ref_t a)
{
	ref_t result;
	trace_inc_indent();

	stack_enter(a);

	if (!e.type ||
		!e.type->eval)
	{
		result = clone_ref(e);
	}
	else
		result = e.type->eval(e, a);

	trace_dec_indent();
	return result;
}
