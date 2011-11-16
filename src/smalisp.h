#ifndef SMALISP_H
#define SMALISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <assert.h>

typedef enum trace_level_ts
{
	TRACE_NONE = 0,
	TRACE_FULL
} trace_level_t;

void set_trace_level(trace_level_t new_level);
void set_trace_file(FILE *fl);
trace_level_t get_trace_level();

/* recognises format tags: %s, %d, %f, %lf, %x, %p and %r for a ref */
void trace(trace_level_t lvl, const char *fmt, ...);
void trace_inc_indent();
void trace_dec_indent();

typedef unsigned char byte_t;

typedef struct gc_object_ts gc_object_t;
typedef struct symbol_ts symbol_t;
typedef struct string_ts string_t;
typedef struct ref_ts ref_t;

typedef struct type_traits_ts type_traits_t;

struct type_traits_ts
{
	ref_t (*eval)(ref_t instance, ref_t context);
	ref_t (*execute)(ref_t instance, ref_t args, ref_t calling_context);
	void (*print)(ref_t instance, FILE *to);
	ref_t (*type_name)(ref_t instance);
	int (*eq)(ref_t a, ref_t b);
	int (*eql)(ref_t a, ref_t b);
	void (*addref)(ref_t instance);
	void (*release)(ref_t instance);
	void (*gc_mark)(ref_t instance);
	void (*gc_release_refs)(ref_t instance);
	void (*gc_free_mem)(ref_t instance);
};

struct ref_ts
{
	const type_traits_t *type;
	union ref_data_ts
	{
		int integer;
		double real;
		symbol_t *symb;
		string_t *str;
		ref_t (*fexec)(ref_t args, ref_t assoc);
		gc_object_t *object; /* cons, macro and closure objects */
	} data;
};

typedef ref_t (*foreign_exec_t)(ref_t args, ref_t assoc);

#define NIL (0)
extern const type_traits_t *string_type;
extern const type_traits_t *symbol_type;
extern const type_traits_t *integer_type;
extern const type_traits_t *foreign_exec_type;
extern const type_traits_t *real_type;
extern const type_traits_t *cons_type;
extern const type_traits_t *macro_type;
extern const type_traits_t *function_type;
extern const type_traits_t *closure_type;
extern const type_traits_t *stack_type;
extern const type_traits_t *stack_frame_type;

/* registers the core functions with a stack frame */
void register_core_lib(ref_t env);

void set_print_file(FILE *fl);
void set_read_file(FILE *fl);

/* returns a symbol ref, with ref count >= 1 [if len == 0, it will use strlen(s)] */
ref_t make_symbol(const char *name, size_t len);

ref_t make_string(const char *s, size_t len);

/* returns a cons ref, with ref count 1 */
ref_t make_cons(ref_t car, ref_t cdr);
void rplaca(ref_t cons, ref_t new_car);
void rplacd(ref_t cons, ref_t new_cdr);
ref_t car(ref_t cons);
ref_t cdr(ref_t cons);
ref_t cadr(ref_t cons);
ref_t caddr(ref_t cons);
ref_t caar(ref_t cons);
ref_t cadar(ref_t cons);

ref_t list(ref_t a, ref_t b);
ref_t list3(ref_t a, ref_t b, ref_t c);
ref_t list_from_array(ref_t *refs, size_t num);

const char *symbol_c_str(ref_t ref);

/* constructs a new function */
ref_t make_function(ref_t param_list, ref_t code, ref_t env);

/* constructs a new macro */
ref_t make_macro(ref_t param_list, ref_t code, ref_t env);

/* constructs a new raw closure */
ref_t make_closure(ref_t param_list, ref_t code, ref_t env);

/* returns the nil reference */
ref_t nil();

/* returns a new integer */
ref_t make_integer(int n);

/* returns a new real */
ref_t make_real(double n);

/* returns a foreign exec ref */
ref_t make_foreign_exec(foreign_exec_t func);

/* returns a new stack frame */
ref_t make_stack(ref_t parent);

/* rebinds the value of name to val in the given stack frame or one of its parent frames
   error if name is currently unbound */
void stack_set(ref_t stack, ref_t name, ref_t val);

/* binds or rebinds the value of name to val in the given stack frame */
void stack_let(ref_t stack, ref_t name, ref_t val);

/* sets up all symbols to enter the given stack */
void stack_enter(ref_t stack);

/* registers a root value with the garbage collector */
void register_gc_root(ref_t o);

/* unregisters a root value with the garbage collector */
void unregister_gc_root(ref_t o);

/* performs a simple mark-sweep garbage collection cycle */
void collect_garbage();

/* increments the ref count for an object */
void add_ref(ref_t ref);

/* increments the ref count for an object, and returns the new reference
   (like a constructor for a ref type in C++ */
ref_t clone_ref(ref_t r);

/* decrements the ref count for an object, and resets the reference to NIL */
void release_ref(ref_t *ref);

/* prints a value */
void print(ref_t val, FILE *to);

/* prints a value, followed by a newline */
void println(ref_t val, FILE *to);

/* reads one SmaLisp value */
ref_t read(FILE *from);

/* evaluates a SmaLisp expression, in the context of an assoc list. */
ref_t eval(ref_t expr, ref_t assoc);

/* tests whether two objects are the same object */
int eq(ref_t a, ref_t b);

/* tests whether two values are equal */
int eql(ref_t a, ref_t b);

/* construct a new list, where each item is the corresponding item
   in the source list, evaluated in environment a */
ref_t map_eval(ref_t list, ref_t assoc);

/* sets the given names and values in the given stack frame */
void map_let(ref_t frame, ref_t names, ref_t vals);

/* evaluate the body of a closure, passing it the specified arguments.
   This does not evaluate the arguments, or the result of the
   closure (but can be run on closures, functions and macros).
   it's used by call() */
ref_t apply(ref_t exec, ref_t args);

/* evaluate a closure, passing it the specified arguments.
   apply will do slightly different things for macros,
   functions and raw closures.
   macros do not evaluate their arguments, but do evaluate
   their result.  functions do evaluate their arguments,
   but do not evaluate their result, and raw closures
   evaluate neither their arguments nor their result. */
ref_t call(ref_t exec, ref_t args, ref_t assoc);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
