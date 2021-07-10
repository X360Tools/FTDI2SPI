
#include "stdafx.h"
#include <windows.h>

#include "XSPI.h"
#include "wrapper_spi.h"

#define GPIO_DIR_XBOX		0x03
#define GPIO_EJ_XBOX		0x02
#define GPIO_XX_XBOX		0x01

unsigned char GPIO_STATUS = 0x00;

unsigned char reverse( unsigned char n )
{
	n = ((n >> 1) & 0x55) | ((n << 1) & 0xaa) ;
	n = ((n >> 2) & 0x33) | ((n << 2) & 0xcc) ;
	n = ((n >> 4) & 0x0f) | ((n << 4) & 0xf0) ;
	return n;
}

void reverse_array( unsigned char *buf, unsigned int size )
{
	unsigned int i;

	for( i=0; i!=size; ++i )
	{
		buf[i] = reverse( buf[i] );
	}
}

void XSPIInit()
{
}

void XSPIPowerUp()
{
}

void XSPIShutdown()
{

}

void XSPIEnterFlashMode()
{
	spi_setGPIO( false, true );
	spi_SetCS( true );
	Sleep(35);	
	spi_setGPIO( false, false );
	spi_SetCS( false );
	Sleep(35);	
	spi_setGPIO( true, true );
	Sleep(35);	
	

}

void XSPILeaveFlashMode()
{

}

void XSPIRead_(unsigned char reg, unsigned char Data[])
{
	unsigned char writeBuf[2] = { (reg << 2) | 1, 0xFF };	
	//6 byte			
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	AddReadOutBuffer( 4*8 );	
	DisableSPIChip();
	
}

void XSPIRead_sync(unsigned char reg, unsigned char Data[])
{
	unsigned char writeBuf[2] = { (reg << 2) | 1, 0xFF };	
		
	ClearOutputBuffer();
	
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	AddReadOutBuffer( 4*8 );	
	DisableSPIChip();

	SetAnswerFast();
	SendBytesToDevice();
	GetDataFromDevice( 4, Data );
	
}

unsigned int XSPIReadWORD_(unsigned char reg)
{
	unsigned char res[2] = {0,0};
	unsigned char writeBuf[2] = { (reg << 2) | 1, 0xFF };	

	ClearOutputBuffer();
		
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	AddReadOutBuffer( 2*8 );	
	DisableSPIChip();

	SetAnswerFast();
	SendBytesToDevice();
	GetDataFromDevice( 2, res );

	return res[0] | ((unsigned int)res[1]<<8);
}

unsigned char XSPIReadBYTE_(unsigned char reg)
{
	unsigned char res;
	unsigned char writeBuf[2] = { (reg << 2) | 1, 0xFF };	
	
	ClearOutputBuffer();
		
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	AddReadOutBuffer( 1*8 );	
	DisableSPIChip();

	SetAnswerFast();
	SendBytesToDevice();
	GetDataFromDevice( 1, &res );
	return res;
}

void XSPIWrite_(unsigned char reg, unsigned char Data[] )
{
	unsigned char writeBuf[5] = { (reg << 2) | 2, 0,0,0,0 };	
	memcpy( &writeBuf[1], Data, 4 );
	
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	DisableSPIChip();

}


void XSPIWrite_sync(unsigned char reg, unsigned char Data[] )
{
	unsigned char writeBuf[5] = { (reg << 2) | 2, 0,0,0,0 };	
	memcpy( &writeBuf[1], Data, 4 );

	ClearOutputBuffer();
	
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	DisableSPIChip();

	SendBytesToDevice();
}

void XSPIWriteWORD_(unsigned char reg, unsigned int Data)
{	
	unsigned char writeBuf[5] = { (reg << 2) | 2, 0,0,0,0 };	
	memcpy( &writeBuf[1], &Data, 4 );

	ClearOutputBuffer();
	
	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );
	DisableSPIChip();

	SendBytesToDevice();
}

void XSPIWrite0_(unsigned char reg)
{
	unsigned char writeBuf[5] = { (reg << 2) | 2, 0,0,0,0 };	

	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );	
	DisableSPIChip();

}

void XSPIWriteBYTE_(unsigned char reg, unsigned char d)
{
	unsigned char writeBuf[5] = { (reg << 2) | 2, d,0,0,0 };	

	EnableSPIChip();
	AddWriteOutBuffer( sizeof(writeBuf)*8, writeBuf );	
	DisableSPIChip();

}