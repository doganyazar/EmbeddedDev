#ifndef DEBUGUTILITIES_H_
#define DEBUGUTILITIES_H_

enum {PRINT_CHARACTER, PRINT_DECIMAL};
#ifdef LOG_ENABLED

#if (LOG_ENABLED >= 3)
#include <ctype.h> //for isprint
#include <stdio.h>

/// //DY : FIX_ME : FOR DEBUG PURPOSE - delete later

static void printBuffer(char* buf,unsigned int len,int type)
{
  unsigned int i = 0;
  printf("--->");
  for ( i=0; i<len ;i++ )
  {
    if ( type == PRINT_CHARACTER )
    {
      if ( isprint(buf[i]) )
      {
        printf("%c",buf[i]);
      }
      else
      {
        printf("(%d)",buf[i]);
      }
    }
    else if ( type == PRINT_DECIMAL )
    {
      printf("(%d)",buf[i]);
    }
  }
  printf("<---\n");
}
#else 
  #define printBuffer(...)
#endif

#else
  #define printBuffer(...)
#endif //LOG_ENABLED

#endif /*DEBUGUTILITIES_H_*/
