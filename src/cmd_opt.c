#include "global.h"

#include "cmd_opt.h"
#include "str.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

static int next_unnamed_opt = -1;
static int wanted_string_opt = -1;
static char error_message_buf[1024];

/*

<command line> ::= <option> <command_line>
               ::= <unnamed param> <command_line>
			   ::=

<option> ::= <long_flag>
		 ::= <long_name_string>
         ::= <short_flag_set>
		 ::= <short_name_string>

<long_flag> ::= '--' <long_flag_name>

<short_flag_set> ::= '-' <short_flag_name_set>
<short_flag_name_set> ::= <short_flag_name> <short_flag_name_set>
                      ::= <short_flag_name>

<long_name_string> ::= '--' <long_opt_name> '=' <string>
<short_name_string> ::= '-' <short_opt_name> '=' <string>
                    ::= '-' <short_opt_name> SPACE <string>
					::= '-' <short_opt_name> <string>

*/

const char *get_error()
{
	return error_message_buf;
}

int find_opt_by_long_name(const char *name, cmd_opt_decl_t opts[])
{
	int i;

	for (i = 0; opts[i].type != CMO_END; ++i)
	{
		if (opts[i].long_name == 0)
			continue;

		if (strcmp(name, opts[i].long_name) == 0)
			return i;
	}

	return -1;
}

int find_opt_by_short_name(char name, cmd_opt_decl_t opts[])
{
	int i;

	for (i = 0; opts[i].type != CMO_END; ++i)
	{
		if (opts[i].short_names == 0)
			continue;

		if (strchr(opts[i].short_names, name))
			return i;
	}

	return -1;
}

int set_flag(cmd_opt_decl_t *opt)
{
	int *var = (int*)opt->var;

	assert(opt->type == CMO_FLAG);

	if (var == 0)
	{
		sprintf(error_message_buf, "Error in option declarator list - no variable provided.");
		return -1;
	}

	*var = 1;
	return 0;
}

int set_string(cmd_opt_decl_t *opt, str_t val)
{
	const char **var = (const char**)opt->var;
	char *s;

	assert(opt->type == CMO_STRING);

	if (var == 0)
	{
		sprintf(error_message_buf, "Error in option declarator list - no variable provided.");
		return -1;
	}

	s = X_MALLOC(val.len + 1);
	memcpy(s, val.c_str, val.len);
	s[val.len] = 0;
	*var = s;

	return 0;
}

void split_on_equals(str_t *src, str_t *name, str_t *val)
{
	size_t i;
	for (i = 0; i < src->len; ++i)
	{
		if (src->c_str[i] == '=')
		{
			str_substr(name, src, 0, (int)i);
			str_substr(val, src, (int)i+1, 0);
			return;
		}
	}
	*name = *src;
	str_init(val, 0);
}

int parse_long_name_option(int *argc, char **argv[], cmd_opt_decl_t opts[])
{
	char *s;
	int result = 0;
	str_t arg, name, val;
	str_init(&name, 0);
	str_init(&val, 0);

#define FREE_AND_RETURN(x) {result = x; goto free_and_return;}

	s = **argv;
	assert(s && s[0] == '-' && s[1] == '-');

	s += 2;

	arg = str_make(s);
	split_on_equals(&arg, &name, &val);

	if (! str_empty(&val))
	{
		int opt = find_opt_by_long_name(name.c_str, opts);
		if (opt == -1)
		{
			sprintf(error_message_buf, "Unknown parameter: %s.", name.c_str);
			FREE_AND_RETURN(-1);
		}
		if (opts[opt].type != CMO_STRING)
		{
			sprintf(error_message_buf, "Didn't expect a value for option %s.", name.c_str);
			FREE_AND_RETURN(-1);
		}
		FREE_AND_RETURN(set_string(&opts[opt], val));
	}
	else
	{
		int opt = find_opt_by_long_name(name.c_str, opts);
		if (opt == -1)
		{
			sprintf(error_message_buf, "Unknown parameter: %s.", name.c_str);
			FREE_AND_RETURN(-1);
		}
		if (opts[opt].type != CMO_FLAG)
		{
			sprintf(error_message_buf, "Expected a value for option %s.", name.c_str);
			FREE_AND_RETURN(-1);
		}
		FREE_AND_RETURN(set_flag(&opts[opt]));
	}

#undef FREE_AND_RETURN

free_and_return:
	str_free(&arg);
	str_free(&name);
	str_free(&val);
	return result;
}

int parse_short_name_option(int *argc, char **argv[], cmd_opt_decl_t opts[])
{
	char *s;
	int result;
	str_t arg, names, val;
	str_init(&names, 0);
	str_init(&val, 0);

#define FREE_AND_RETURN(x) {result = x; goto free_and_return;}

	s = **argv;
	assert(s && s[0] == '-');

	++s;

	arg = str_make(s);
	split_on_equals(&arg, &names, &val);
	if (! str_empty(&val))
	{
		int opt;
		if (names.len != 1)
		{
			sprintf(error_message_buf, "LOL I CAN'T BE BOTHERED TO WRITE ANY MORE ERROR MESSAGES (1)");
			FREE_AND_RETURN(-1);
		}
		opt = find_opt_by_short_name(names.c_str[0], opts);
		if (opt == -1)
		{
			sprintf(error_message_buf, "Unknown option: %c", names.c_str[0]);
			FREE_AND_RETURN(-1);
		}
		if (opts[opt].type != CMO_STRING)
		{
			sprintf(error_message_buf, "Didn't expect a value for option %s.", names.c_str);
			FREE_AND_RETURN(-1);
		}
		FREE_AND_RETURN(set_string(&opts[opt], val));
	}
	else
	{
		int opt = find_opt_by_short_name(names.c_str[0], opts);

		if (names.len > 1)
		{
			size_t i;
			for (i = 0; i < names.len; ++i)
			{
				char n = names.c_str[i];
				int opt = find_opt_by_short_name(n, opts);
				if (opt == -1)
				{
					sprintf(error_message_buf, "Unknown option: %c", n);
					FREE_AND_RETURN(-1);
				}
				if (set_flag(&opts[opt]) == -1)
				{
					FREE_AND_RETURN(1);
				}
			}
			FREE_AND_RETURN(0);
		}
		else
		{
			
			if (opts[opt].type == CMO_FLAG)
			{
				FREE_AND_RETURN(set_flag(&opts[opt]));
			}
			else
			{
				--*argc; ++*argv;
				if (*argc == 0)
				{
					sprintf(error_message_buf, "Expected a value for option %c", names.c_str[0]);
					FREE_AND_RETURN(-1);
				}
				val = str_make(**argv);
				FREE_AND_RETURN(set_string(&opts[opt], val));
			}
		}
	}

#undef FREE_AND_RETURN

free_and_return:
	str_free(&arg);
	str_free(&names);
	str_free(&val);
	return result;
}

int parse_option(int *argc, char **argv[], cmd_opt_decl_t opts[])
{
	char *c;

	assert(*argc > 0);
	c = **argv;
	assert(*c = '-');
	++c;

	if (*c == '-')
	{
		/* long name flag, or long name argument */
		++c;
		return parse_long_name_option(argc, argv, opts);
	}
	else
		return parse_short_name_option(argc, argv, opts);
}

int parse_string(int *argc, char **argv[], cmd_opt_decl_t opts[])
{
	static first_run = 1;
	static next_unnamed_opt = -1;
	
	int result;
	str_t s;
	
	if (first_run)
	{
		int i;
		first_run = 0;

		next_unnamed_opt = -1;
		for (i = 0; opts[i].type != CMO_END; ++i)
		{
			if (opts[i].short_names == 0 &&
				opts[i].long_name == 0 &&
				opts[i].type == CMO_STRING)
			{
				next_unnamed_opt = i;
				break;
			}
		}
	}

	if (next_unnamed_opt == -1)
		return 0;

	s = str_make(**argv);
	result = set_string(&opts[next_unnamed_opt], s);
	next_unnamed_opt++;
	if (opts[next_unnamed_opt].short_names != 0 ||
		opts[next_unnamed_opt].long_name != 0 ||
		opts[next_unnamed_opt].type != CMO_STRING)
	{
		next_unnamed_opt = -1;
	}

	str_free(&s);
	return result;
}

int parse_command_line(int argc, char *argv[], cmd_opt_decl_t opts[])
{
	char *c;

	--argc; ++argv;

	if (argc == 0)
		return 0;

	c = argv[0];
	if (*c == '-')
	{
		if (parse_option(&argc, &argv, opts) == -1)
			return -1;
		return parse_command_line(argc, argv, opts);
	}
	else
	{
		if (parse_string(&argc, &argv, opts) == -1)
			return -1;
		return parse_command_line(argc, argv, opts);
	}
}

void print_option_list(cmd_opt_decl_t opts[])
{
	cmd_opt_decl_t *decl;

	printf("Parameters:\n");

	for (decl = opts; decl->type != CMO_END; ++decl)
	{
		printf("  ");
		if (!decl->short_names && !decl->long_name)
		{
			if (decl->type == CMO_STRING)
				printf("<string>");
			printf("\n      ");
			if (decl->required)
				printf("[required] ");
			if (decl->usage_msg)
				printf("%s\n", decl->usage_msg);
		}
		else
		{
			if (decl->short_names)
			{
				printf("-%s", decl->short_names);
				if (decl->type == CMO_STRING)
					printf("<string>");
			}
			if (decl->short_names && decl->long_name)
				printf(" or ");
			if (decl->long_name)
			{
				printf("--%s", decl->long_name);
				if (decl->type == CMO_STRING)
					printf("=<string>");
			}

			printf("\n      ");
			if (decl->required)
				printf("[required] ");
			if (decl->usage_msg)
				printf("%s\n", decl->usage_msg);
		}
	}
}

/* usage messages are in the form:

Usage: app-name [ -optionalshortnamedflags ] [ --each-optional-long-named-flag ] 

*/
