/* vim: set ts=4 sts=4 sw=4 noet ai: */
#ifndef CMD_OPT_H
#define CMD_OPT_H

typedef enum cmd_opt_type_ts
{
	CMO_END = 0,
	CMO_FLAG = 1,
	CMO_STRING
} cmd_opt_type;

typedef struct cmd_opt_decl_ts cmd_opt_decl_t;
struct cmd_opt_decl_ts
{
	const char *short_names;
	const char *long_name;
	cmd_opt_type type;
	void *var;
	int required;
	const char *usage_msg;
};

int parse_command_line(int argc, char *argv[], cmd_opt_decl_t opts[]);
const char *get_error();
void print_option_list(cmd_opt_decl_t opts[]);

/* example usage:

cmd_opt_decl_t cmd_opt_decls[] =
{
  {"o", "output", CMO_STRING, &output_fname, 0, "specifies the eval output file."},
  {"q", "quiet", CMO_FLAG, &quiet_flag, 0, "Don't output the results of top-level evals."},
  {0, "trace-file", CMO_STRING, &trace_file_fname, 0, "Specify a file to output traces and stack dumps to."},
  {0, 0, CMO_STRING, &input_fname, 0, "The script to run."},
  {0}
};

if (! parse_command_line(argc, argv, cmd_opt_decls))
{
    print_usage(cmd_opt_decls);
    exit(1);
}

*/

#endif
