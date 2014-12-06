/***
 * Excerpted from "Test-Driven Development for Embedded C",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/jgade for more book information.
***/
#include <stdio.h>
#include "fake_timer.h"



//Need these extern vars since they are defined in Time_Native or else linking fails.
//TODO to be removed later when we get rid of that global vars
int _sem_SetClock;

static time_t var_fake_time;
void Time_Create(void)
{
}

void Time_Destroy(void)
{
}


void FakeTime_SetTime(time_t fake_time){
	var_fake_time = fake_time;
}

int Time_getTime(void){
	return var_fake_time;
}


unsigned int ReadCoreTimer( void){
	printf("Not Fake Implemented \n");
	return 1;
}

void WriteCoreTimer(unsigned int timer){
	printf("Not Fake Implemented \n");
}

void randfuncs( int* RF_T_Array, int* RF_RH_Array, char* RF_N_Array ){
	printf("Not Fake Implemented \n");
}
