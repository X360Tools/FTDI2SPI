
#include "stdafx.h"
#include <windows.h>

#include "XSPI.h"

extern void ClearOutputBuffer( void );
extern void SetAnswerFast( void );
extern void SendBytesToDevice( void );
extern void GetDataFromDevice( unsigned int dwNumDataBitsToRead, unsigned char ReadDataBuffer[] );

bool XNANDWaitReady(unsigned int timeout)
{
	do {
		if (!(XSPIReadBYTE_(0x04) & 0x01))
			return true;
	} while (timeout--);

	return false;
}

unsigned int XNANDGetStatus()
{
	return XSPIReadWORD_(0x04);
}

void XNANDClearStatus()
{
	unsigned char tmp[4] = { 0,2,0,0 };

	XSPIRead_sync(4, tmp);	
	XSPIWrite_sync(4, tmp);	

}

unsigned int XNANDReadStart(unsigned int block)
{
	unsigned int res;
	unsigned int tries = 0x1000;

	XNANDClearStatus();

	ClearOutputBuffer();	

	XSPIWriteWORD_(0x0C, block << 9);
	XSPIWriteBYTE_(0x08, 0x03);

	SendBytesToDevice();

	if (!XNANDWaitReady(0x1000))
		return 0x8011;

	res = 0;

	ClearOutputBuffer();
	XSPIWrite0_(0x0C);
	SendBytesToDevice();

	return res;
}

void XNANDReadProcess(unsigned char * pData, unsigned char Words ) {
	unsigned int len = Words;

	ClearOutputBuffer();
	while (Words--) {
		XSPIWrite0_(0x08);
		XSPIRead_(0x10, pData);
	}

	SetAnswerFast();
	SendBytesToDevice();
	GetDataFromDevice( len*4, pData );
}

unsigned int XNANDErase(unsigned int block)
{
	unsigned char tmp[4];

	XNANDClearStatus();

	XSPIRead_sync(0, tmp);	
	tmp[0] |= 0x08;
	XSPIWrite_sync(0, tmp);

	ClearOutputBuffer();	

	XSPIWriteWORD_(0x0C, block << 9);
	XSPIWriteBYTE_(0x08, 0xAA);
	XSPIWriteBYTE_(0x08, 0x55);
	XSPIWriteBYTE_(0x08, 0x5);

	SendBytesToDevice();

	if (!XNANDWaitReady(0x1000))
		return 0x8003;

	return XNANDGetStatus();	
}

void XNANDWriteStart()
{

	ClearOutputBuffer();
	XSPIWrite0_(0x0C);
	SendBytesToDevice();

}

void XNANDWriteProcess(unsigned char Buffer[], unsigned char Words) 
{

	ClearOutputBuffer();	
	while (Words--) {
		XSPIWrite_(0x10, Buffer);
		XSPIWriteBYTE_(0x08, 0x01);
		Buffer += 4;
	}
	SendBytesToDevice();

}

unsigned int XNANDWriteExecute(unsigned int block) 
{
	unsigned int tries = 0x1000;

	ClearOutputBuffer();	
	XSPIWriteWORD_(0x0C, block << 9);
	XSPIWriteBYTE_(0x08, 0x55);
	XSPIWriteBYTE_(0x08, 0xAA);
	XSPIWriteBYTE_(0x08, 0x4);
	SendBytesToDevice();

	if (!XNANDWaitReady(0x1000))
		return 0x8021;

	return XNANDGetStatus();	

}