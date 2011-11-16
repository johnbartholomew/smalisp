#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>

#include "mem.h"

#define LOG_ERROR(msg) fprintf(stderr, "! ERROR in %s (%s:%d): %s\n", __FUNCTION__, __FILE__, __LINE__, (msg)); assert(0)
#define LOG_ERROR_X(msg, x) fprintf(stderr, "! ERROR in %s (%s:%d): " msg "\n", __FUNCTION__, __FILE__, __LINE__, x); assert(0)
#define LOG_WARNING(msg) fprintf(stderr, "WARNING in %s (%s:%d): %s\n", __FUNCTION__, __FILE__, __LINE__, (msg))
#define LOG_WARNING_X(msg, x) fprintf(stderr, "WARNING in %s (%s:%d): " msg "\n", __FUNCTION__, __FILE__, __LINE__, x)

#endif
