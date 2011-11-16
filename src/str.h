/* str.h, the interface to the high-level string functions

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

*/

/* <sys/types.h> must be included before str.h */

/* Include only once */
#ifndef _STR_H_
#define _STR_H_

/* Allow in C++ code */
#ifdef __cplusplus
extern "C" {
#endif

/* Information about the string */
typedef struct str_s
{
    /* The normal c type string underlying this */
    char *c_str;
    /* The length of this string */
    size_t len;
    /* Allocated memory for the pointer at c_str, might be bigger than len+1 */
    size_t size;
	/* Whether the string lib owns the string data */
	int is_owner;

#ifdef STR_CACHE_HASH
	/* Hash of the string, for comparisons and putting in a hash table.
	   If it's 0, then the hash hasn't been calculated yet.  Always get the
	   hash with the str_hash() function, not through this field. */
	unsigned long hash;
#endif
} str_t;


/* Initialize a string */
str_t str_init(str_t *str, int initial_size);
/* Free up all the memory allocated in the string */
void str_free(str_t *str);
/* Turn this normal c string into a str_t, uses the same pointer and NO extra
    memory is allocated */
str_t str_make(char *src);

/* Fill up the string from another string */
void str_copy(str_t *str, const str_t *src);
/* Fill up the string from a char* string of given size, 0 will autocalculate */
void str_fill(str_t *str, const char *buf, size_t len);
/* Fill up the string from a given character */
void str_char(str_t *str, const char ch);
/* Fill up the string from a given integer */
void str_int(str_t *str, const int i);
/* Fill up the string from a given unsigned integer */
void str_uint(str_t *str, const unsigned int ui);
/* Fill up the string from a given double precision floating point number */
void str_double(str_t *str, const double lf);

/* Reset the string to empty */
void str_clr(str_t *str);
/* Check if the string is empty */
int str_empty(const str_t *str);
/* Hash this string and return a value for it */
unsigned long str_hash(str_t *str);
/* Compact the used memory for this string */
void str_compact(str_t *str);

/* Compare a string with another string, 0 if mathed */
int str_compare(const str_t *str1, const str_t *str2);
/* Comapre with another string but ignore the case of the letters */
int str_compare_icase(const str_t *str1, const str_t *str2);
/* Return the relative position of some substring in the string */
int str_pos(const str_t *str, const str_t *sub);

/* Turn the src into all lower case and return in str, pointers can be same */
void str_lower(str_t *str, const str_t *src);
/* Turn the src into all upper case and return in str, pointers can be same */
void str_upper(str_t *str, const str_t *src);

/* Take the substring of src and fill up str, pointers can be same, negative
    numbers will wrap from the end */
void str_substr(str_t *str, const str_t *src, int start, int end);
/* Insert the src string into the position given in the str, pointers can be
    same, negative numbers will wrap from the end */
void str_insert(str_t *str, const str_t *src, int pos);
/* Delete the given section of src string and put the rest in str, pointers can
    be same, negative numbers will wrap from the end */
void str_delete(str_t *str, const str_t *src, int pos, int count);
/* Chomp the string, i.e. remove extra spaces/illegal chars from the ends,
    pointers can be same */
void str_chomp(str_t *str, const str_t *src);

/* Append a single character to str (eg, for string building) */
void str_append_char(str_t *str, char c);

/* Find the first occurance of this character, -1 if not found */
int str_indexof(const str_t *str, const char ch);
/* Convert to integer */
int str_to_int(const str_t *str);
/* Convert to double */
double str_to_double(const str_t *str);


#ifdef __cplusplus
}
#endif

#endif /* !_STR_H_ */
