#ifndef LOGGER_H_
#define LOGGER_H_

/*Logging is enabled via LOG_ENABLED Macro*/
#ifdef LOG_ENABLED

/*Includes*/
#include <stdio.h> /*need to declare here because of basic_logger*/
#include <stdarg.h>

/*Debug levels specifying how much information will be printed*/
enum eLevel{L_NONE, L_ERR, L_INFO, L_DBG};

/*
 * Helps the logger by printing log record number, log level etc.
 * */
void logger_helper(unsigned char level, const char* func_name);

/*
 * Defines functions depending on whether persistent or basic logger is wanted.
 * */
#ifndef PERSISTENT_LOG
  #define basic_logger(...) printf(__VA_ARGS__)
#else /*PERSISTENT_LOG*/
  void persistent_logger(char* fmt, ...);
  #define basic_logger(...) persistent_logger(__VA_ARGS__)
#endif /*PERSISTENT_LOG*/

/*Main Logger Macro
 *__func__ added by C99 standard and it is not a macro though;
the preprocessor does not know the name of the current function.*/
#define TOOLS_LOGGER(level,...) \
do \
{ \
  logger_helper(level,__func__); \
  basic_logger(__VA_ARGS__); \
} while(0)


/*Logger macro definitions*/
#define LOG_ERR(...) TOOLS_LOGGER(L_ERR, __VA_ARGS__)

#if (LOG_ENABLED >= 2)
  #define LOG_INFO(...) TOOLS_LOGGER(L_INFO, __VA_ARGS__)
  #if (LOG_ENABLED >= 3)
    #define LOG_DBG(...) TOOLS_LOGGER(L_DBG, __VA_ARGS__)
  #else
    #define LOG_DBG(...)
  #endif
#else
  #define LOG_INFO(...)
  #define LOG_DBG(...)
#endif

#else /*LOG_ENABLED*/
  #define LOG_ERR(...)
  #define LOG_INFO(...)
  #define LOG_DBG(...)
#endif /*LOG_ENABLED*/

#endif /*LOGGER_H_*/
