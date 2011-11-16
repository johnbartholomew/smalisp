#ifndef MEM_H
#define MEM_H

void *x_malloc(size_t size, const char *func, const char *file, unsigned long line, const char *msg);
void x_free(void *p, const char *func, const char *file, unsigned long line, const char *msg);

#if defined(_DEBUG) || defined(DEBUG)

#define malloc() error_malloc
#define free() error_free

#define X_MALLOC(s) (x_malloc((s), __FUNCTION__, __FILE__, __LINE__, 0))
#define XX_MALLOC(s, msg) (x_malloc((s), __FUNCTION__, __FILE__, __LINE__, (msg)))
#define X_FREE(p) if (!p) {} else { x_free((p), __FUNCTION__, __FILE__, __LINE__, 0); (p) = 0; }
#define XX_FREE(p, msg) if (!p) {} else { x_free((p), __FUNCTION__, __FILE__, __LINE__, (msg)); (p) = 0; }

#else /* release build */

#define X_MALLOC(s) (malloc(s))
#define XX_MALLOC(s, msg) (malloc(s))
#define X_FREE(p) if (!p) {} else free(p)
#define XX_FREE(p, msg) if (!p) {} else free(p)

#endif

#endif
