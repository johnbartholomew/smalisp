#include "global.h"

#include "str.h"

#include "smalisp.h"

#include "gc.h"
#include "symbol.h"
#include "cons.h"

#define MAX_PEEK_BUF (64)
static char peek_buf[MAX_PEEK_BUF] = {0};
static int peek_head = -1;

static FILE *source_file;

static void _unget(int c)
{
	assert(peek_head < MAX_PEEK_BUF - 1); /* TODO: this should be real error checking, not an assert */

	peek_buf[++peek_head] = c;
}

static void _unget_str(char *s, size_t n)
{
	assert((int)n < MAX_PEEK_BUF - peek_head); /* TODO: this should be real error checking, not an assert */

	--n;
	while (n != -1) /* can't use > -1, because n is unsigned */
		peek_buf[++peek_head] = s[n--];
}

static int _get()
{
	if (peek_head == -1)
		return getc(source_file);
	else
		return peek_buf[peek_head--];
}

static int _peek()
{
	int c;
	if (peek_head == -1)
	{
		c = getc(source_file);
		_unget(c);
	}
	else
		c = peek_buf[peek_head];
	return c;
}

static void _skip_comment()
{
	int c;
	c = _peek();
	assert(c == ';');
	_get(); /* skip the comment marker */
	while (1)
	{
		c = _peek();
		if (c == '\n')
		{
			_get();
			break;
		}
		else
			_get();
	}
}

static void _skip_whitespace()
{
	int c;
	while (1)
	{
		c = _peek();
		if (c == ';')
			_skip_comment();
		else if (isspace(c))
			_get();
		else
			break;
	}
}

static ref_t _read_cons_cdr();
static ref_t _read_cons();
static ref_t _read_quoted();
static ref_t _read_number();
static ref_t _read_symbol();

static ref_t _read_cons_cdr()
{
	int c;
	
	_skip_whitespace();
	c = _peek();

	if (c == -1)
	{
		/* TODO: ERROR here - input ended without a closing bracket */
		LOG_ERROR("input ended without a closing bracket");
		return nil();
	}
	else if (c == ')') /* end of a cons list, so the cdr is nil */
		return nil();
	else if (c == '.') /* dotted notation, eg, (a . b) */
	{
		_get(); /* skip the dot */
		return read(source_file);
	}
	else /* the cons continues... */
	{
		ref_t cons, car, cdr;

		car = read(source_file);
		cdr = _read_cons_cdr();
		cons = make_cons(car, cdr);
		release_ref(&car);
		release_ref(&cdr);
		return cons;
	}
}

static ref_t _read_cons()
{
	int c;

	_get(); /* skip the opening bracket */
	_skip_whitespace();

	c = _peek();
	if (c == -1)
	{
		/* TODO: ERROR - opening bracket but no closing bracket */
		LOG_ERROR("opening bracket with no closing bracket");
		return nil();
	}
	else if (c == ')')
	{
		_get(); /* eat the ) */
		return nil(); /* empty cons is nil */
	}
	else /* normal cons */
	{
		ref_t cons, car, cdr;
		car = read(source_file);
		cdr = _read_cons_cdr();
		if (_peek() == ')')
			_get();
		else
		{
			release_ref(&car);
			release_ref(&cdr);
			/* TODO: ERROR - no closing bracket */
			LOG_ERROR("no closing bracket");
			return nil();
		}
		cons = make_cons(car, cdr);
		release_ref(&car);
		release_ref(&cdr);
		return cons;
	}
}

static ref_t _read_quoted()
{
	ref_t quotesym, val, quotecons;
	int c;

	c = _get();
	assert(c == '\'');

	quotesym = make_symbol("quote", 0);
	val = read(source_file);
	quotecons = list(quotesym, val);

	release_ref(&quotesym);
	release_ref(&val);

	return quotecons;
}

static ref_t _read_quasiquoted()
{
	ref_t qquotesym, val, qquotecons;
	int c;

	c = _get();
	assert(c == '`');

	qquotesym = make_symbol("quasiquote", 0);
	val = read(source_file);
	qquotecons = list(qquotesym, val);

	release_ref(&qquotesym);
	release_ref(&val);

	return qquotecons;
}

static ref_t _read_unquoted()
{
	ref_t unquotesym, val, unquotecons;
	int c;

	c = _get();
	assert(c == ',');

	unquotesym = make_symbol("unquote", 0);
	val = read(source_file);
	unquotecons = list(unquotesym, val);

	release_ref(&unquotesym);
	release_ref(&val);

	return unquotecons;
}

static ref_t _read_number()
{
	static char smallstr_buf[128] = {0};
	int c;
	str_t buf;
	ref_t ref;

	int has_digits = 0, is_int = 1;

	c = _peek();

	assert(c == '-' || c == '+' || c == '.' || (c >= '0' && c <= '9'));
	
	smallstr_buf[0] = 0;
	buf.c_str = smallstr_buf;
	buf.len = 0;
	buf.size = 128;
#ifdef STR_CACHE_HASH
	buf.hash = 0;
#endif

	/* get the sign */
	if (c == '-' || c == '+')
	{
		str_append_char(&buf, _get());
		c = _peek();

		if (c < '0' || c > '9') /* not really a number (sign but no value) */
			goto not_really_a_number;
	}

	if (c >= '0' && c <= '9') has_digits = 1;

	/* get the main part of the number */
	while (c >= '0' && c <= '9')
	{
		str_append_char(&buf, _get());
		c = _peek();
	}

	/* check for a decimal point */
	if (c == '.')
	{
		is_int = 0;
		str_append_char(&buf, _get());
		c = _peek();

		if (c >= '0' && c <= '9') has_digits = 1;

		if (! has_digits) /* not really a number (decimal point, but not digits anywhere) */
			goto not_really_a_number;

		/* get the decimal places */
		while (c >= '0' && c <= '9')
		{
			str_append_char(&buf, _get());
			c = _peek();
		}
	}

	/* check for an exponent */
	if (c == 'e' || c == 'E')
	{
		if (! has_digits) /* not really a number; got to have digits somewhere before an exponent */
			goto not_really_a_number;

		is_int = 0;
		str_append_char(&buf, _get());
		c = _peek();

		/* get the sign of the exponent */
		if (c == '+' || c == '-')
		{
			str_append_char(&buf, _get());
			c = _peek();
		}
	
		if (c < '0' || c > '9') /* not really a number (exponent but no exponent value) */
			goto not_really_a_number;

		/* get the exponent */
		while (c >= '0' && c <= '9')
		{
			str_append_char(&buf, _get());
			c = _peek();
		}
	}

	if (is_int)
		ref = make_integer(str_to_int(&buf));
	else
		ref = make_real(str_to_double(&buf));

	if (buf.c_str != smallstr_buf)
		str_free(&buf);

	return ref;

not_really_a_number:
	_unget_str(buf.c_str, buf.len);
	if (buf.c_str != smallstr_buf)
		str_free(&buf);
	return nil();
}

static ref_t _read_string()
{
	static char smallstr_buf[128] = {0};
	int c;
	str_t buf;
	ref_t str;

	c = _peek();
	assert(c == '"');
	_get(); /* skip the opening quote */

	smallstr_buf[0] = 0;
	buf.c_str = smallstr_buf;
	buf.len = 0;
	buf.size = 128;
	buf.is_owner = 0;
#ifdef STR_CACHE_HASH
	buf.hash = 0;
#endif

	c = _peek();
	while (c != '"' && c != -1)
	{
		if (c == '\\')
		{
			_get(); /* skip the escape char */
			c = _get();
			switch (c)
			{
			case '0': str_append_char(&buf, '\0'); break;
			case 'r': str_append_char(&buf, '\r'); break;
			case 'n': str_append_char(&buf, '\n'); break;
			case 'b': str_append_char(&buf, '\b'); break;
			case 't': str_append_char(&buf, '\t'); break;
			default: str_append_char(&buf, c); break;
			}
		}
		else
			str_append_char(&buf, _get());

		c = _peek();
	}
	
	if (c == '"')
		_get(); /* skip the closing quote */
	else
	{
		LOG_ERROR("Unclosed quoted string");
		str_free(&buf);
		return nil();
	}

	str = make_string(buf.c_str, buf.len);

	str_free(&buf);

	return str;
}

static ref_t _read_symbol()
{
	static char smallstr_buf[128] = {0};
	int c;
	str_t buf;
	ref_t symb;

	c = _peek();
	assert(!isspace(c) && (c < '0' || c > '9') && c != -1 && c != '\r' && c != '\n');

	smallstr_buf[0] = 0;
	buf.c_str = smallstr_buf;
	buf.len = 0;
	buf.size = 128;
#ifdef STR_CACHE_HASH
	buf.hash = 0;
#endif

	if (c == '|')
	{
		_get(); /* skip the opening pipe */
		c = _peek();
		while (c != '|' && c != -1)
		{
			if (c == '\\')
			{
				_get(); /* skip the escape char */
				c = _get();
				switch (c)
				{
				case '0': str_append_char(&buf, '\0'); break;
				case 'r': str_append_char(&buf, '\r'); break;
				case 'n': str_append_char(&buf, '\n'); break;
				case 'b': str_append_char(&buf, '\b'); break;
				case 't': str_append_char(&buf, '\t'); break;
				default: str_append_char(&buf, c); break;
				}
			}
			else
				str_append_char(&buf, _get());

			c = _peek();
		}
		if (c == '|')
			_get(); /* skip the closing pipe */
	}
	else
	{
		while (!isspace(c) && c != ')' && c != -1 && c != '\r' && c != '\n')
		{
			if (c == '\\')
			{
				_get(); /* skip the escape char */
				c = _get();
				switch (c)
				{
				case '0': str_append_char(&buf, '\0'); break;
				case 'r': str_append_char(&buf, '\r'); break;
				case 'n': str_append_char(&buf, '\n'); break;
				case 'b': str_append_char(&buf, '\b'); break;
				case 't': str_append_char(&buf, '\t'); break;
				default: str_append_char(&buf, c); break;
				}
			}
			else
				str_append_char(&buf, _get());

			c = _peek();
		}
	}

	symb = make_symbol(buf.c_str, buf.len);

	if (buf.c_str != smallstr_buf)
		str_free(&buf);

	return symb;
}

ref_t read(FILE *in)
{
	int c;

	source_file = in;
	
	_skip_whitespace();

	c = _peek();
	switch (c)
	{
	case '(':
		return _read_cons();
	case '\'':
		return _read_quoted();
	case '`':
		return _read_quasiquoted();
	case ',':
		return _read_unquoted();
	case -1:
	case '\r':
	case '\n':
		return nil();
	case ')': /* eat it. TODO: ERROR */
		_get();
		LOG_ERROR("closing bracket with no matching opening bracket");
		return nil();
	case '"':
		return _read_string();
	case '-': case '+': case '.':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		{
			ref_t n = _read_number();
			if (n.type != NIL)
				return n;
			else /* otherwise, it's a symbol */
				return _read_symbol();
		}
	default:
		return _read_symbol();
	}
}
