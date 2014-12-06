//- ------------------------------------------------------------------
//-    Copyright (c) James W. Grenning -- All Rights Reserved         
//-    For use by owners of Test-Driven Development for Embedded C,   
//-    and attendees of Renaissance Software Consulting, Co. training 
//-    classes.                                                       
//-                                                                   
//-    Available at http://pragprog.com/titles/jgade/                 
//-        ISBN 1-934356-62-X, ISBN13 978-1-934356-62-3               
//-                                                                   
//-    Authorized users may use this source code in your own          
//-    projects, however the source code may not be used to           
//-    create training material, courses, books, articles, and        
//-    the like. We make no guarantees that this source code is       
//-    fit for any purpose.                                           
//-                                                                   
//-    www.renaissancesoftware.net james@renaissancesoftware.net      
//- ------------------------------------------------------------------


#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "atc_servercom.h"
#include "fake_comm.h"
#include "common_test_utils.h"
#include "fake_timer.h"

#include "GenericTypeDefs.h"

#include "utils.h"
#include "utils_asciihex.h"
#include "server_comm.h"
}

TEST_GROUP(Communication)
{
	char IMEI[16];
	char IMSI[16];

	unsigned char command;

    void setup()
    {
    	Fake_Comm_Create();

    	sprintf(IMEI, "VN_000000000001");
    	sprintf(IMSI, "678901234567890");

    	time_t faketime = 5;
    	FakeTime_SetTime(faketime);

    	command = 0;
    }

    void teardown()
    {
    }
};

TEST(Communication, Hello)
{
	//TODO: weird to set this here. Move somewhere else later!
	_posix_time = Time_getTime();

	sendHello(IMEI, IMSI);
	char* buf = Fake_Comm_Get_Buffer();
//	int index = Fake_Comm_Get_Buffer_Size();

	//TODO: parameterize - hello(2) protocolversion firmwareversion time imei imsi nodeid
	STRCMP_EQUAL("02 03 " FIRMWAREVERSION " 00000005 VN_000000000001 678901234567890 ATC_GW001\n", buf);
}

TEST(Communication, Bye)
{
	sendBye();
	char* buf = Fake_Comm_Get_Buffer();
	int index = Fake_Comm_Get_Buffer_Size();

	/*BYE=19 => In ASCII HEX '1''3'*/
	LONGS_EQUAL(3, index);
	BYTES_EQUAL('1', buf[0]);
	BYTES_EQUAL('3', buf[1]);
	BYTES_EQUAL('\n', buf[2]);
}

TEST(Communication, GetCommand)
{
    const char* p = "#04";
	Fake_Comm_Set_Rec_Buffer(p, strlen(p));
	getCommandNr(&command);
	BYTES_EQUAL(DATA2NODE, command); //4

	p = "#0e ";
	Fake_Comm_Set_Rec_Buffer(p, strlen(p));
	getCommandNr(&command);
	BYTES_EQUAL(CLKSET, command); //14

	p = "11";
	Fake_Comm_Set_Rec_Buffer(p, strlen(p));
	getCommandNr(&command);
	BYTES_EQUAL(GETLOG, command); //17
}

TEST(Communication, GetClockSet)
{
    const char* p = "51dbcb54\n";
	Fake_Comm_Set_Rec_Buffer(p, strlen(p));
	time_t expected = 0x51dbcb54;
	LONGS_EQUAL(expected, getClkSet());
}

TEST_GROUP(Conversion)
{
	BYTE convert;
	UINT8_VAL val;
    void setup()
    {
    	convert = 0;
    }
    void teardown()
    {
    }
};

TEST(Conversion, Ascii2HexConversion) //8 to 4 bits
{
	// 'A' -> 00xa
	convert = 'A';
	asciihex2binChar(&convert);
	BYTES_EQUAL(0x0a, convert);

	convert = '5';
	asciihex2binChar(&convert);
	BYTES_EQUAL(0x05, convert);

	convert = 'F';
	asciihex2binChar(&convert);
	BYTES_EQUAL(0x0f, convert);
}

TEST(Conversion, Hex2AsciiConversion) //8 to 16 bit
{
	// 0x01 -> '0''1'
	val.Val = 0x01;
	UINT16 expected = ((UINT16)'0') << 8 | ((UINT16)'1');
	LONGS_EQUAL(expected, binChar2AsciiHex(val).Val);

	// 0x8c -> '8''c'
	val.Val = 0x8c;
	expected = ((UINT16)'8') << 8 | ((UINT16)'C');
	LONGS_EQUAL(expected, binChar2AsciiHex(val).Val);
}


TEST_GROUP(Utils)
{
    void setup()
    {
    }
    void teardown()
    {
    }
};

TEST(Utils, Read)
{
	uint8_t p1[] = "abcd\r\n";
	uint8_t p2[] = "abcd";
	POINTERS_EQUAL(&p1[5], multi_strchr(p1, (uint8_t*)"\n"));
	POINTERS_EQUAL(&p1[4], multi_strchr(p1, (uint8_t*)"\r"));

	uint8_t* result = multi_strchr(p2, (uint8_t*)"\n");
	POINTERS_EQUAL(NULL, result);

	const uint8_t buf[] = "123 ab10";
	const uint8_t* end;
	uint32_t out;

	end = read_hex(buf, &out);
	POINTERS_EQUAL(buf+3, end);
	LONGS_EQUAL(0x123, out);

	end = read_hex(end, &out);
	POINTERS_EQUAL(buf+strlen((char*)buf), end);
	LONGS_EQUAL(0xab10, out);


	BYTES_EQUAL(0xab, asciihexbytes_to_byte(p1[0], p1[1]));
	BYTES_EQUAL(0x30, asciihexbytes_to_byte('3', '0'));
}


//TEST(Utils, Parse_Par)
//{
//	char extended[] =
//			"// Testing file for extended .par-file.\n" \
//			"@401B8100 abb2 cce3	0123 4567	89ab cdef	// These bytes will NOT be converted\n"\
//			"401B8200 abb2 cce3	0123 4567	89ab cdef	4567 89ab	cdef 0123	// These bytes will be converted\n"\
//			"$";
//
//	util_parse_extended(extended);
////	p = util_parse_line(p);
//}
