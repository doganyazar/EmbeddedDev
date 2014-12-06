/*
 * common_test_utils.c
 *
 *  Created on: Aug 7, 2013
 *      Author: dogan
 */

#include <stdio.h>
#include <string.h>
#include "common_test_utils.h"

void print_buffer(char* buf, unsigned int  size) {
	int i=0;
	char out[5];
	printf("Str: ");
	for(;i<size;i++){
		char c = buf[i];

		if (c > ' ' && c <= '~'){
			sprintf(out, "%c", c);
		} else if (c < ' '){
			sprintf(out, "(%d)", (int)c);
		} else {
			switch(c){
			case ' ':
				sprintf(out, "(SP)");
				break;
			default:
				sprintf(out, "( )");
			}
		}
		printf("%s ", out);
	}
	printf("\n");
}

void print_buffer_hex(char* buf, unsigned int  size) {
	int i=0;
	printf("Hex: ");
	for(;i<size;i++){
		printf("%x ", (unsigned char)buf[i]);
	}
	printf("\n");
}

void print_buffer_dec(char* buf, unsigned int  size) {
	int i=0;
	printf("Dec: ");
	for(;i<size;i++){
		printf("%d ", (int)buf[i]);
	}
	printf("\n");
}

void clear_buffer(char* buf, int size){
	memset(buf, 0, size);
}
