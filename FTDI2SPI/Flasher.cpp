
#include "stdafx.h"

#include <stdint.h>

#include "Flasher.h"
#include "XNAND.h"
#include "sfc.h"
#include "XSPI.h"

unsigned int gNextBlock;
unsigned char gWORDsLeft;
unsigned int gBYTEsWritten;

unsigned int gGlobalStatus    = 0x00;

void FlashReadStatus(unsigned char Status[]);
void FlashDataInit(unsigned char FlashConfig[]);
void FlashDataDeInit(void);
void PowerUp(void);
void Shutdown(void);
void Update(void);
void FlashDataErase( unsigned int reg );
void FlashDataRead( unsigned char Data[], unsigned int Arg, unsigned int Words );
void FlashDataWrite( unsigned char Data[], unsigned int Arg, unsigned int Words );

void PowerUp() 
{
	XSPILeaveFlashMode();
	XSPIPowerUp();
}

void Shutdown()
{
	XSPILeaveFlashMode();
	XSPIShutdown();
}

void Update()
{
}

void FlashReadStatus(unsigned char Status[])
{	
	Status[0] = ((unsigned char *)&gGlobalStatus)[0];
	Status[1] = ((unsigned char *)&gGlobalStatus)[1];	
}

void FlashDataDeInit()
{
	XSPILeaveFlashMode();
}

void FlashDataErase( unsigned int reg )
{
	gGlobalStatus = XNANDErase( reg );	
}

void FlashDataInit( unsigned char FlashConfig[] )
{
	XSPIEnterFlashMode();
	
	XSPIRead_sync(0, FlashConfig);
	XSPIRead_sync(0, FlashConfig);
}

void NandReadCB(unsigned char * pData, unsigned int len)
{
	len /= 4;

	while (len) {
		unsigned char readnow;

		if (!gWORDsLeft) {
			gGlobalStatus |= XNANDReadStart(gNextBlock);
			gNextBlock++;
			gWORDsLeft = 0x84;
		}

		readnow = (len < gWORDsLeft)?len:gWORDsLeft;		

		XNANDReadProcess(pData, readnow);
		pData+=(readnow*4);
		gWORDsLeft-=readnow;
		len-=readnow;
	}
}

void FlashDataRead( unsigned char * pData, unsigned int Block, unsigned int Words )
{
	gGlobalStatus = 0;
	gWORDsLeft = 0;
	gNextBlock = Block;

	NandReadCB( pData, Words );
}

void FlashDataWrite( unsigned char Data[], unsigned int Block, unsigned int Words )
{
	unsigned char *pData = Data;
	unsigned char len;
	static bool FlashErase;

	if( (Block%Sfc.PageCountInBlock) == 0 ) {
		gNextBlock = Block;
		gGlobalStatus = XNANDErase(gNextBlock);		
		gWORDsLeft = 0x84;
		gBYTEsWritten = 0;
		XNANDWriteStart();
		FlashErase = true;
	}
	
	len = Words / 4;
	
	while (len) {
		unsigned char writeNow = len > gWORDsLeft?gWORDsLeft:len;

		XNANDWriteProcess(pData, writeNow);
		pData += writeNow*4;
		len -= writeNow;
		gWORDsLeft -= writeNow;

		if (gWORDsLeft == 0) {
			gGlobalStatus |= XNANDWriteExecute(gNextBlock);
			gNextBlock++;
			gWORDsLeft = 0x84;
			XNANDWriteStart();
			gBYTEsWritten += 0x210;
		}
	}
}

void fixECC(unsigned char *pagedata, unsigned char *ECC)
{
    int i=0, val=0;

    unsigned long v;

	unsigned long *lpagedata = (unsigned long *) pagedata;
	
    for (i = 0; i < 0x1066; i++)
    {
		if ((i & 0x1F) == 0)
		{
			if (i == 0x1000)
				 lpagedata = (unsigned long*)ECC;
			v = ~*lpagedata++;

		}
		val ^= v & 1;
		v >>= 1;
		if (val & 1)
				val ^= 0x6954559;
		val >>= 1;
    }

    val = ~val;

    *(ECC+0xC) = (val << 6) & 0xC0;
    *(ECC+0xD) = (val >> 2) & 0xFF;
    *(ECC+0xE) = (val >> 10) & 0xFF;
    *(ECC+0xF) = (val >> 18) & 0xFF;
}

// for big block
void fixBB(unsigned char *blockdata)
{
	unsigned int i = 0;
	for(i = 0; i < 64; i++)
	{
		fixECC(blockdata + (i * 0x840) + 0x000, blockdata + (i * 0x840) + 0x800);
		fixECC(blockdata + (i * 0x840) + 0x200, blockdata + (i * 0x840) + 0x810);
		fixECC(blockdata + (i * 0x840) + 0x400, blockdata + (i * 0x840) + 0x820);
		fixECC(blockdata + (i * 0x840) + 0x600, blockdata + (i * 0x840) + 0x830);
	}
}

// for small block
void fixSB(unsigned char *blockdata)
{
	unsigned int i = 0;
	fixECC( blockdata, blockdata + 0x200 );
}


void fixSpare_ECC(unsigned char *blockdata, uint32_t block_num ) 
{
	unsigned int i = 0;
	uint32_t BlockVersion = 0;

	uint32_t block_tmp = block_num / Sfc.PageCountInBlock;

	if(Sfc.LargeBlock)
	{
		blockdata[Sfc.PageSize + 0x5] = (char) (BlockVersion >> 0) & 0xFF;
		blockdata[Sfc.PageSize + 0x3] = (char) (BlockVersion >> 8) & 0xFF;
		blockdata[Sfc.PageSize + 0x4] = (char) (BlockVersion >> 16) & 0xFF;
		blockdata[Sfc.PageSize + 0x6] = (char) (BlockVersion >> 24) & 0xFF;

		blockdata[Sfc.PageSize + 0x2] = (char) (block_tmp >> 8) & 0xFF;
		blockdata[Sfc.PageSize + 0x1] = (char) (block_tmp >> 0) & 0xFF;

		blockdata[Sfc.PageSize + 0x0] = 0xFF;
	}
	else
	{
		blockdata[Sfc.PageSize + 0x2] = (char) (BlockVersion >> 0) & 0xFF;
		blockdata[Sfc.PageSize + 0x3] = (char) (BlockVersion >> 8) & 0xFF;
		blockdata[Sfc.PageSize + 0x4] = (char) (BlockVersion >> 16) & 0xFF;
		blockdata[Sfc.PageSize + 0x6] = (char) (BlockVersion >> 24) & 0xFF;
	
		blockdata[Sfc.PageSize + 0x1] = (char) (block_tmp >> 8) & 0xFF;
		blockdata[Sfc.PageSize + 0x0] = (char) (block_tmp >> 0) & 0xFF;

		blockdata[Sfc.PageSize + 0x5] = 0xFF;
	}

	*(uint32_t*)(blockdata + (i * 0x210) + 0x20C) = 0x00000000;
	
	fixSB( blockdata );
}
