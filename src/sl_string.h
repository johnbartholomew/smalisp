#ifndef SL_STRING_H
#define SL_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

struct string_ts
{
	unsigned long rc;
	unsigned long hash;
	size_t len;
};

const char *string_c_str(string_t *str);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
