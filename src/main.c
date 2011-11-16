#include "global.h"

#include "smalisp.h"
#include "stack.h"
#include "symbol.h"
#include "closure.h"
#include "core_lib.h"

#include "str.h"
#include "cmd_opt.h"

static char *output_fname = 0;
static int quiet_flag = 0;
static int help_flag = 0;
static int stats_flag = 0;
static char *trace_file_fname = 0;
static char *input_fname = 0;

static cmd_opt_decl_t cmd_opt_decls[] =
{
	{"h", "help", CMO_FLAG, &help_flag, 0, "Print this usage message."},
	{"o", "output", CMO_STRING, &output_fname, 0, "Specifies the eval output file."},
	{"q", "quiet", CMO_FLAG, &quiet_flag, 0, "Don't output the results of top-level evals."},
	{"s", "stats", CMO_FLAG, &stats_flag, 0, "Enable tracking certain statistics"},
	{0, "trace-file", CMO_STRING, &trace_file_fname, 0, "Specify a file to output traces and stack dumps to."},
	{0, 0, CMO_STRING, &input_fname, 0, "The script to run."},
	{0}
};

/* types of command line options:
   - single char flags.  can be combined in a single block.
   - single char params.
   - multi-char flags.
   - multi-char params.
   - list of arguments
*/

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

static int finished = 0;
static FILE *trace_fl = 0;

ref_t slfe_exit(ref_t args, ref_t assoc)
{
	finished = 1;
	return nil();
}

ref_t slfe_trace(ref_t args, ref_t assoc)
{
	if (trace_fl)
	{
		ref_t result;
		trace_level_t tl = get_trace_level();
		set_trace_level(TRACE_FULL);
		result = slfe_do(args, assoc);
		set_trace_level(tl);
		return result;
	}
	else
		return slfe_do(args, assoc);
}

ref_t slfe_no_trace(ref_t args, ref_t assoc)
{
	if (trace_fl)
	{
		ref_t result;
		trace_level_t tl = get_trace_level();
		set_trace_level(TRACE_NONE);
		result = slfe_do(args, assoc);
		set_trace_level(tl);
		return result;
	}
	else
		return slfe_do(args, assoc);
}

ref_t slfe_dump_stack(ref_t args, ref_t assoc)
{
	ref_t arg, arge;

	if (trace_fl == 0)
		return nil();

	arg = car(args);
	if (arg.type != NIL)
	{
		arge = eval(arg, assoc);
		if (arge.type == stack_type)
			stack_debug_print((stack_t*)arge.data.object, trace_fl);
		else
			LOG_WARNING("dump-stack called with an invalid argument.");
		release_ref(&arge);
	}
	else
		stack_debug_print((stack_t*)assoc.data.object, trace_fl);

	release_ref(&arg);

	return nil();
}

__int64 rdtsc()
{
__asm cpuid  // flush the pipe
__asm rdtsc  // read time stamp register
}

ref_t slfe_profile(ref_t args, ref_t assoc)
{
	if (trace_fl)
	{
		ref_t result, block_name, block;
		__int64 start, end;

		block_name = car(args);
		block = cdr(args);

		start = rdtsc();
		result = slfe_do(block, assoc);
		end = rdtsc();

		trace(TRACE_NONE, "Timing for block %r: %Ld", block_name, end - start);

		release_ref(&block);
		release_ref(&block_name);
		return result;
	}
	else
		return slfe_do(args, assoc);
}

void print_usage()
{
	printf("Usage: smalisp [-h or --help] [-q or --quiet] [-s or --stats] [--trace-file=<trace-output-file>] [-o or --output=<output-file>] [ <input-file> ]\n\n");
	print_option_list(cmd_opt_decls);
	printf("\n");
}

int main(int argc, char *argv[])
{
	int result = 0;
	ref_t val, answer, assoc, name;
	FILE *input_fl = stdin, *output_fl = stdout;
	clock_t start_time, end_time;
	
#define FREE_AND_RETURN(x) {result = x; goto free_and_return;}

	if (parse_command_line(argc, argv, cmd_opt_decls) == -1)
	{
		printf("%s\n\n", get_error());
		print_usage();
		FREE_AND_RETURN(1);
	}

	if (help_flag)
	{
		print_usage();
		FREE_AND_RETURN(0);
	}

	if (input_fname)
	{
		input_fl = fopen(input_fname, "r");
		if (input_fl == 0)
		{
			printf("Could not open input file %s\n", input_fname);
			FREE_AND_RETURN(1);
		}
	}

	if (output_fname)
	{
		output_fl = fopen(output_fname, "w");
		if (output_fl == 0)
		{
			printf("Could not open output file %s\n", output_fname);
			FREE_AND_RETURN(1);
		}
	}

	if (trace_file_fname)
	{
		trace_fl = fopen(trace_file_fname, "w");
		if (trace_fl == 0)
		{
			printf("Could not open trace file %s\n", trace_file_fname);
			FREE_AND_RETURN(1);
		}
	}

	assoc = make_stack(nil());
	register_gc_root(assoc);
	stack_enter(assoc);

	name = make_symbol("t", 0);
	stack_let(assoc, name, name);
	release_ref(&name);
	
	register_core_lib(assoc);

	REG_FN(exit, assoc);
	REG_FN(trace, assoc);
	REG_FN(profile, assoc);
	REG_NAMED_FN("no-trace", slfe_no_trace, assoc);
	REG_NAMED_FN("dump-stack", slfe_dump_stack, assoc);

	if (trace_fl)
		set_trace_file(trace_fl);

	start_time = clock();

	finished = 0;
	while (! finished)
	{
		if (input_fl == stdin)
			printf("> ");

		val = read(input_fl);
		answer = eval(val, assoc);
		release_ref(&val);
		if (!quiet_flag)
		{
			println(answer, output_fl);
			fflush(output_fl);
		}
		release_ref(&answer);
		collect_garbage();
	}

	stack_enter(nil());
	unregister_gc_root(assoc);
	release_ref(&assoc);

	collect_garbage();

	end_time = clock();

	if (trace_fl)
		fprintf(trace_fl, "Total time taken: %f seconds\n", (float)(end_time - start_time) / (float)CLOCKS_PER_SEC);

#undef FREE_AND_RETURN

free_and_return:
	if (trace_fl && stats_flag)
		fprintf(trace_fl, "Total symbol evals: %d; total stack switches: %d\n", symbol_eval_count, stack_switch_count);

	if (input_fl != stdin) fclose(input_fl);
	if (output_fl != stdout) fclose(output_fl);
	if (trace_fl) fclose(trace_fl);

	if (input_fname) X_FREE(input_fname);
	if (output_fname) X_FREE(output_fname);
	if (trace_file_fname) X_FREE(trace_file_fname);

	return result;
}
