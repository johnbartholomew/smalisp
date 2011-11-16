#ifndef CORE_LIB_H
#define CORE_LIB_H

#define REG_FN(n, e) \
	{ ref_t name = make_symbol(#n, 0); \
	  ref_t val = make_foreign_exec(slfe_##n); \
	  stack_let(e, name, val); \
	  release_ref(&name); release_ref(&val); }

#define REG_NAMED_FN(n, f, e) \
	{ ref_t name = make_symbol(n, 0); \
	  ref_t val = make_foreign_exec(f); \
	  stack_let(e, name, val); \
	  release_ref(&name); release_ref(&val); }

ref_t slfe_quote(ref_t args, ref_t assoc);
ref_t slfe_eq(ref_t args, ref_t assoc);
ref_t slfe_eql(ref_t args, ref_t assoc);
ref_t slfe_cond(ref_t args, ref_t assoc);
ref_t slfe_car(ref_t args, ref_t assoc);
ref_t slfe_cdr(ref_t args, ref_t assoc);
ref_t slfe_atom(ref_t args, ref_t assoc);
ref_t slfe_macro(ref_t args, ref_t assoc);
ref_t slfe_fn(ref_t args, ref_t assoc);
ref_t slfe_closure(ref_t args, ref_t assoc);
ref_t slfe_set(ref_t args, ref_t assoc);
ref_t slfe_env_set(ref_t args, ref_t assoc);
ref_t slfe_let(ref_t args, ref_t assoc);
ref_t slfe_env_let(ref_t args, ref_t assoc);
ref_t slfe_cons(ref_t args, ref_t assoc);
ref_t slfe_do(ref_t args, ref_t assoc);
ref_t slfe_scope(ref_t args, ref_t assoc);
ref_t slfe_apply(ref_t args, ref_t assoc);
ref_t slfe_macro_expand(ref_t args, ref_t assoc);
ref_t slfe_closure_code(ref_t args, ref_t assoc);
ref_t slfe_closure_env(ref_t args, ref_t assoc);
ref_t slfe_closure_plist(ref_t args, ref_t assoc);
ref_t slfe_make_closure(ref_t args, ref_t assoc);
ref_t slfe_print(ref_t args, ref_t assoc);
ref_t slfe_read(ref_t args, ref_t assoc);
ref_t slfe_eval(ref_t args, ref_t assoc);
ref_t slfe_get_env(ref_t args, ref_t assoc);
ref_t slfe_type(ref_t args, ref_t assoc);
ref_t slfe_add(ref_t args, ref_t assoc);
ref_t slfe_sub(ref_t args, ref_t assoc);
ref_t slfe_mul(ref_t args, ref_t assoc);
ref_t slfe_div(ref_t args, ref_t assoc);
ref_t slfe_mod(ref_t args, ref_t assoc);
ref_t slfe_bitand(ref_t args, ref_t assoc);
ref_t slfe_bitor(ref_t args, ref_t assoc);
ref_t slfe_bitxor(ref_t args, ref_t assoc);
ref_t slfe_bitnot(ref_t args, ref_t assoc);
ref_t slfe_gc_collect(ref_t args, ref_t assoc);

#endif
