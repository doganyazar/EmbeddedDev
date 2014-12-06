/***
 * Excerpted from "Test-Driven Development for Embedded C",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/jgade for more book information.
***/
#ifndef D_Fake_Comm_H
#define D_Fake_Comm_H

/**********************************************************
 *
 * Comm is responsible for ...
 *
 **********************************************************/

#include "atc_servercom.h"

void Fake_Comm_Create(void);
void Fake_Comm_Destroy(void);

char* Fake_Comm_Get_Buffer(void);
int Fake_Comm_Get_Buffer_Size(void);

void Fake_Comm_Set_Rec_Buffer(const char* buf, int size);

#endif  /* D_Fake_Comm_H */
