/***
 * Excerpted from "Test-Driven Development for Embedded C",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/jgade for more book information.
***/
#ifndef D_FAKETIMER_H
#define D_FAKETIMER_H

/**********************************************************
 *
 * FakeTime is responsible for providing a
 * test stub for Time
 *
 **********************************************************/

#include <time.h>
#include "time_clock.h"

void FakeTime_SetTime(time_t fake_time);

#endif  /* D_FAKETIMER_H */
