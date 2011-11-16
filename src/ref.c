#include "global.h"

#include "smalisp.h"
#include "ref.h"
#include "gc.h"

ref_t nil()
{
	ref_t ref;
	memset(&ref, 0, sizeof(ref_t));
	ref.type = NIL;
	return ref;
}

void add_ref(ref_t ref)
{
	if (ref.type && ref.type->addref)
		ref.type->addref(ref);
}

ref_t clone_ref(ref_t r)
{
	add_ref(r);
	return r;
}

void release_ref(ref_t *ref)
{
	assert(ref);
	if (ref->type && ref->type->release)
		ref->type->release(*ref);
	
	*ref = nil();
}

void ref_gc_mark(ref_t ref)
{
	if (ref.type && ref.type->gc_mark)
		gc_mark(ref.data.object);
}
