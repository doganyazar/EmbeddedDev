#ifndef TRACER_H_
#define TRACER_H_

#include "GenericTypeDefs.h"

#include <stdio.h>

/*LOG LEVELS
 * 0 : No log
 * 1 : Error only
 * 2 : Error and debug
 * */

#ifndef TRACE_ENABLED
  #define TRACE_ENABLED 0
#endif /*TRACE_ENABLED*/

/*Default log level is ERROR*/
#ifndef LOG_LEVEL
  #define LOG_LEVEL 1
#endif /*LOG_LEVEL*/


void tracer_init(void);

#define LOGGER(...) \
do \
{ \
  printf(__VA_ARGS__);\
  printf("\n\r");\
} while(0)

#if (LOG_LEVEL >= 1)
  #define LOGE(...) LOGGER(__VA_ARGS__)
  #if (LOG_LEVEL >= 2)
    #define LOGD(...) LOGGER(__VA_ARGS__)
  #else /*(LOG_LEVEL >= 2)*/
    #define LOGD(...)
  #endif /*(LOG_LEVEL >= 2)*/
#else /*(LOG_LEVEL >= 1)*/
  #define LOGE(...)
  #define LOGD(...)
#endif /*(LOG_LEVEL >= 1)*/


#if TRACE_ENABLED

#define TRACE(...) LOGGER(__VA_ARGS__)

#define TRACE_CALL_ENTER() \
do \
{ \
  TRACE(">> %s()", __func__);\
} while(0)

/*
 * ## is not used as token paste operator here. It is a GNU CPP extension. Details: http://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html
 * Second, the ‘##’ token paste operator has a special meaning when placed between a comma and a variable argument. If you write
 * #define eprintf(format, ...) fprintf (stderr, format, ##__VA_ARGS__) and the variable argument is left out then the comma before the ‘##’ will be deleted.
 * */
#define TRACE_CALL_ENTER_PARAM_FMT(X, ...) \
	TRACE(">> %s(" X ")", __func__, ##__VA_ARGS__)

#define TRACE_CALL_ENTER_PARAM_INT(X) \
	TRACE(">> %s(%d)", __func__, (int)X)

#define TRACE_CALL_PARAM_INT(NAME, X) \
	TRACE("__%s(): %s=%d", __func__, NAME, X)
#define TRACE_CALL_PARAM_FMT(X, ...) \
    TRACE("__%s():" X, __func__, ##__VA_ARGS__)

#define TRACE_CALL_EXIT() \
	TRACE("<< %s()", __func__)

#define TRACE_CALL_EXIT_PARAM_FMT(X, ...) \
	TRACE("<< %s()=>" X, __func__, ##__VA_ARGS__)

#define TRACE_CALL_EXIT_PARAM_INT(X) \
	TRACE("<< %s()=>%d", __func__, (int)X)

#else /*TRACE_ENABLED*/

#define TRACE_CALL_ENTER()
#define TRACE_CALL_ENTER_PARAM_FMT(X, ...)
#define TRACE_CALL_ENTER_PARAM_INT(X)
#define TRACE_CALL_PARAM_INT(NAME, X)
#define TRACE_CALL_PARAM_FMT(X, ...)
#define TRACE_CALL_EXIT()
#define TRACE_CALL_EXIT_PARAM_FMT(X, ...)
#define TRACE_CALL_EXIT_PARAM_INT(X)
#endif /*TRACE_ENABLED*/


void LogByte(BYTE byte);

#endif /* TRACER_H_ */
