/* str.h, the implementation of the high-level string functions

  Copyright (C) 2002 Pouya Larjani

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Pouya Larjani
  pouya@rogers.com

  -----

  As according to point 2: THIS VERSION OF THE SOURCE CODE HAS BEEN ALTERED
  by John Bartholomew.

  The changes are:
  + The addition of a (#define controlled) 'hash' field to cache the string hash for
     string buffers.  If STR_CACHE_HASH is defined, then this extra hash field
     will be included in str_t, and the str_hash() function will use it to cache
     the string hash (note that the hash field is not kept up to date all the time,
     it is *only* a cache, so when retreiving the hash, you should always use
     the str_hash() function which will recalculate the hash if necessary)
  + The addition of an initial_size parameter to the str_init() function.  This will
     resize the buffer of the string so that it has extra capacity to begin with
     (so that if you are building a string, it won't have to keep reallocating memory
     as the string gradually gets larger)
  + The addition of the str_append_char() function, which can be used to build up
     the string by appending single characters to the end of it (for example, if you're
     performing tokenisation, this is often useful)
  + The inclusion of "mem.h", and replacments of calls to malloc and free with
     calls to the macros X_MALLOC and X_FREE, in order to interface with the
     aforementioned memory checking utility functions.

*/


#include "global.h"
#include <sys/types.h>

#include "str.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996) /* 'foo' was declared deprecated [yeah, right, by whom, exactly?] */
#endif

#define STR_FALSE       0
#define STR_TRUE        1


/* Resize the string */
int str_resize(str_t *str, size_t reqlen)
{
    size_t nsize;
    char *nstr;

    if (reqlen <= str->size)
        return STR_TRUE;

    /* Double the size each time, amortized cost is 2 */
    nsize = 1;
    while (nsize < reqlen)
        nsize <<= 1;

    nstr = (char*)X_MALLOC(nsize);
    if (!nstr)
        return STR_FALSE;

    if (str->c_str)
    {
        memcpy(nstr, str->c_str, str->size);
		if (str->is_owner)
			X_FREE(str->c_str);
    }

    str->c_str = nstr;
	str->is_owner = 1;
    str->size = nsize;

    return STR_TRUE;
}

/* Initialize a string */
str_t str_init(str_t *str, int initial_size)
{
    static str_t s;

    if (!str)
        str = &s;

    str->c_str = NULL;
    str->len = 0;
    str->size = 0;
	str->is_owner = 1;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif

	if (initial_size)
	{
		if (str_resize(str, initial_size))
			str->c_str[0] = '\0';
	}

    return *str;
}

/* Free up all the memory allocated in the string */
void str_free(str_t *str)
{
    if (str && str->c_str)
    {
		if (str->is_owner)
			X_FREE(str->c_str);
        str->c_str = NULL;
        str->len = 0;
        str->size = 0;
#ifdef STR_CACHE_HASH
		str->hash = 0;
#endif
    }
}

/* Turn this normal c string into a str_t, uses the same pointer and NO extra
    memory is allocated */
str_t str_make(char *src)
{
    str_t str;

	str.is_owner = 0;
    str.c_str = src;
    str.len = strlen(src);
    str.size = str.len + 1;
#ifdef STR_CACHE_HASH
	str.hash = 0;
#endif

    return str;
}

/* Fill up the string from another string */
void str_copy(str_t *str, const str_t *src)
{
    if (!str_resize(str, src->len + 1))
        return;

    memcpy(str->c_str, src->c_str, src->len + 1);
    str->len = src->len;
#ifdef STR_CACHE_HASH
	str->hash = src->hash;
#endif
}

/* Fill up the string from a char* string of given size, 0 will autocalculate */
void str_fill(str_t *str, const char *buf, size_t len)
{
    if (len == 0)
        len = strlen(buf);

    if (!str_resize(str, len+1))
        return;

    memcpy(str->c_str, buf, len);
    str->c_str[len] = '\0';
    str->len = len;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

/* Fill up the string from a given character */
void str_char(str_t *str, const char ch)
{
    if (!str_resize(str, 2))
        return;

    str->c_str[0] = ch;
    str->c_str[1] = '\0';
    str->len = 1;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/* Fill up the string from a given integer */
void str_int(str_t *str, const int i)
{
    char buf[32];
    snprintf(buf, 32, "%i", i);
    str_fill(str, buf, 0);
}

/* Fill up the string from a given unsigned integer */
void str_uint(str_t *str, const unsigned int ui)
{
    char buf[32];
    snprintf(buf, 32, "%u", ui);
    str_fill(str, buf, 0);
}

/* Fill up the string from a given double precision floating point number */
void str_double(str_t *str, const double lf)
{
    char buf[32];
    snprintf(buf, 32, "%g", lf);
    str_fill(str, buf, 0);
}

#ifdef _MSC_VER
#undef snprintf
#endif

/* Reset the string to empty */
void str_clr(str_t *str)
{
    if (str->c_str)
        X_FREE(str->c_str);
    str->c_str = NULL;
    str->len = str->size = 0;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif

    if (str_resize(str, 1))
        str->c_str[0] = 0;
}

/* Check if the string is empty */
int str_empty(const str_t *str)
{
    return (str->len == 0);
}

/* Hash this string and return a value for it */
unsigned long str_hash(str_t *str)
{
#ifdef STR_CACHE_HASH
	if (str->hash)
		return str->hash;
	else
	{
#endif
	    unsigned long ans = 0;
		char *a = (char*)&ans;
	    const char *s = str->c_str;
		size_t i;

		for (i=0; i<str->len; i++)
			a[i % sizeof(unsigned long)] ^= tolower(*s++);

#ifdef STR_CACHE_HASH
	    return str->hash = ans;
	}
#else
		return ans;
#endif
}

/* Compact the used memory for this string */
void str_compact(str_t *str)
{
    char *nstr;
    if ((str->len + 1 >= str->size) || !str->c_str)
        return;

    /* Shrink */
    nstr = (char*)X_MALLOC(str->len+1);
    if (!nstr)
        return;

    memcpy(nstr, str->c_str, str->len+1);
    X_FREE(str->c_str);
    str->c_str = nstr;
    str->size = str->len+1;
}

/* Compare a string with another string, 0 if mathed */
int str_compare(const str_t *str1, const str_t *str2)
{
    return strcmp(str1->c_str, str2->c_str);
}

/* Comapre with another string but ignore the case of the letters */
int str_compare_icase(const str_t *str1, const str_t *str2)
{
    int a, b;
    const char *src = str1->c_str, *dst = str2->c_str;

    do {
        a = tolower(*dst++);
        b = tolower(*src++);
    } while (a && (a == b));

    return (a-b);
}

/* Return the relative position of some substring in the string */
int str_pos(const str_t *str, const str_t *sub)
{
    return (int)(strstr(str->c_str, sub->c_str) - str->c_str);
}

/* Turn the src into all lower case and return in str, pointers can be same */
void str_lower(str_t *str, const str_t *src)
{
    size_t i;
    if (str_resize(str, src->len + 1))
        for (i=0; i<=src->len; i++)
            str->c_str[i] = tolower(src->c_str[i]);
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

/* Turn the src into all upper case and return in str, pointers can be same */
void str_upper(str_t *str, const str_t *src)
{
    size_t i;
    if (str_resize(str, src->len + 1))
        for (i=0; i<=src->len; i++)
            str->c_str[i] = toupper(src->c_str[i]);
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

/* Take the substring of src and fill up str, pointers can be same, negative
    numbers will wrap from the end */
void str_substr(str_t *str, const str_t *src, int start, int end)
{
    str_t ans = str_init(NULL, 0);

    if (start < 0)
        start += (int)src->len;
    if (end <= 0)
        end += (int)src->len;

    if ((start >= 0) && (end >= 0) && (start <= (int)src->len) &&
        (end <= (int)src->len) && (end >= start) &&
        str_resize(&ans, end-start+1))
    {
        memcpy(ans.c_str, src->c_str + start, end-start);
        ans.c_str[end-start] = '\0';
        ans.len = end-start;
    }

    str_free(str);
    str->c_str = ans.c_str;
    str->len = ans.len;
    str->size = ans.size;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

/* Insert the src string into the position given in the str, pointers can be
    same, negative numbers will wrap from the end */
void str_insert(str_t *str, const str_t *src, int pos)
{
    str_t ans = str_init(NULL, 0);

    if (pos <= 0)
        pos += (int)str->len;

    if ((pos >= 0) && (pos <= (int)str->len) &&
        str_resize(&ans, str->len + src->len + 1))
    {
        memcpy(ans.c_str, str->c_str, pos);
        memcpy(ans.c_str + pos, src->c_str, src->len);
        memcpy(ans.c_str + pos + src->len, str->c_str + pos, str->len - pos);
        ans.len = str->len + src->len;
        ans.c_str[ans.len] = '\0';
    }

    str_free(str);
    str->c_str = ans.c_str;
    str->len = ans.len;
    str->size = ans.size;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

/* Delete the given section of src string and put the rest in str, pointers can
    be same, negative numbers will wrap from the end */
void str_delete(str_t *str, const str_t *src, int pos, int count)
{
    str_t ans = str_init(NULL, 0);

    if (pos <= 0)
        pos += (int)src->len;

    if ((pos >= 0) && (pos + count <= (int)src->len) && (count >= 0) &&
        str_resize(&ans, src->len - count + 1))
    {
        memcpy(ans.c_str, src->c_str, pos);
        memcpy(ans.c_str + pos, src->c_str + pos + count,
            src->len - pos - count);
        ans.len = src->len - count;
        ans.c_str[ans.len] = '\0';
    }

    str_free(str);
    str->c_str = ans.c_str;
    str->len = ans.len;
    str->size = ans.size;
#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
}

/* Chomp the string, i.e. remove extra spaces/illegal chars from the ends,
    pointers can be same */
void str_chomp(str_t *str, const str_t *src)
{
    size_t st = 0, en = src->len;

    while (!isgraph(src->c_str[st]) && (st < en))
        st++;
    while (!isgraph(src->c_str[en-1]) && (en > 0))
        en--;

    str_substr(str, src, (int)st, (int)en);
}

/* Append a single character to str (eg, for string building) */
void str_append_char(str_t *str, char c)
{
    if (str_resize(str, str->len + 1))
    {
		str->c_str[str->len] = c;
		++str->len;
		str->c_str[str->len] = '\0';

#ifdef STR_CACHE_HASH
	str->hash = 0;
#endif
    }
}

/* Find the first occurance of this character, -1 if not found */
int str_indexof(const str_t *str, const char ch)
{
    char *i = strchr(str->c_str, ch);

    if (i == NULL)
        return -1;
    else
        return (int)(i - str->c_str);
}

/* Convert to integer */
int str_to_int(const str_t *str)
{
    return atoi(str->c_str);
}

/* Convert to double */
double str_to_double(const str_t *str)
{
    return atof(str->c_str);
}
