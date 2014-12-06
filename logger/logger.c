#ifdef LOG_ENABLED

#include "contiki.h"
#include "logger.h"

void logger_helper(unsigned char level, const char* func_name)
{
  static unsigned int log_count = 0;
  basic_logger("%u;%d;",++log_count,level);

  #ifdef LOG_DETAILS
    basic_logger("%s;%hu;",func_name, clock_seconds());
  #endif /*LOG_DETAILS*/
}

/*PERSISTENT_LOG declarations*/
#ifdef PERSISTENT_LOG
  #include "cfs/cfs.h"
  #include "cfs/cfs-coffee.h"
  #include "dev/leds.h"

  #define PAGE_NAME "TUSE"

  #define TEMP_LOG_RECORDS_SIZE 1000
  char temp_log_records[TEMP_LOG_RECORDS_SIZE];
  unsigned short temp_log_records_index;

void persistent_logger(char* fmt, ...)
{
  va_list args;
  va_start( args, fmt );

  unsigned char size_left = TEMP_LOG_RECORDS_SIZE-temp_log_records_index;
  if (size_left > 1)
  {
    unsigned short number_of_bytes = vsnprintf(temp_log_records + temp_log_records_index, size_left, fmt, args);
    temp_log_records_index += number_of_bytes;
  }

  va_end( args );
}

/* Write in the file system
 * returns 1 if everything was ok, 0 if not
 * */
int write_log_string(char *log_record, unsigned short size)
{
  printf("Page name = '%s', Size of the data = %d \n",PAGE_NAME,size);
  if ( size > 0 )
  {
    leds_on(LEDS_GREEN);
    int wfd = cfs_open(PAGE_NAME, CFS_WRITE | CFS_APPEND);
    if(wfd < 0) {
      printf("#ERROR! Impossible to open for write (cfs_open returned %d)!#\n",wfd);
      cfs_close(wfd);
      return 0;
    }

    int r = cfs_write(wfd, log_record, size);  /*Try to write some data in the buffer*/
    if(r != size) {
      printf("#ERROR! Impossible to write data in the buffer (cfs_write returned %d < %d)!#\n",r,size);
      cfs_close(wfd);
      return 0;
    }
    else{
      printf("Write returns %d (size was %d)\n",r,size);
    }
    cfs_close(wfd);
    leds_off(LEDS_GREEN);
  }

  return 1;
}

/*this process is started in contiki-sky-main.c if PERSISTENT_LOG defined.*/
PROCESS(logger_process, "Logger process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(logger_process, ev, data)
{
  PROCESS_BEGIN();

  printf("$$$$$$$$$$$$$$$$$$$ LOGGER PROCESS \n");

  while(1)
  {
    static struct etimer et;

    etimer_set(&et, CLOCK_SECOND * 60);

    PROCESS_WAIT_UNTIL(etimer_expired(&et));

    if(write_log_string(temp_log_records, strlen(temp_log_records)) == 0){
      printf("#ERROR! Impossible to write the data into the file system! Write returned 0!#\n");
    }

    /*reset the buffer*/
    temp_log_records_index = 0;
    temp_log_records[0] = 0;
  }

  PROCESS_END();
}

#endif /*PERSISTENT_LOG*/

#endif /*LOG_ENABLED*/
