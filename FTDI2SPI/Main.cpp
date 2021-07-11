#include "stdafx.h"

#include <windows.h>
#include "wrapper_spi.h"
#include "Flasher.h"
#include "sfc.h"
#include <sys/stat.h>
#include <string>

//#include <process.h>
//#include <stdio.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// unused, but lets leave it here
#define PRINT_PROGRESS printf( "\r%0.1f%%", ((block_flash-StartBlock)*100)/(float)(block_flash_max-StartBlock) )
#define PRINT_BLOCKS printf( "\rBlock: %X", (block_flash/32))
                     
#include <iostream>
#include <fstream>
using namespace std;

unsigned int block_flash = 0;
unsigned int block_size;
unsigned int flashConfig = 0;

extern "C"
{
	__declspec(dllexport) int spi(int mode, int size, char* file) {
		unsigned char* flash_xbox;
		unsigned int addr_raw = 0;
		unsigned int StartBlock = 0;
		unsigned int CountBlock = 0;
		block_flash = 0;
		unsigned int block_flash_max = 2048 * size;

		bool Wrong_arg = false;
		bool EraseEnable = false;
		bool ReadEnable = false;
		bool WriteEnable = false;
		bool PatchECCEnable = false;
		string FileName = file;

		if (mode == 0) { // Get Flash Config Only
			block_size = 0x210;
			ReadEnable = true;
			StartBlock = 0;
			block_flash_max = 32;
		}
		else if (mode == 1) { // -r
			block_size = 0x210;
			ReadEnable = true;
		}
		else if (mode == 2) { // -R
			block_size = 0x200;
			ReadEnable = true;
		}
		else if (mode == 3) { // -w
			block_size = 0x210;
			WriteEnable = true;
		}
		else if (mode == 4) { // +w
			PatchECCEnable = true;
			block_size = 0x210;
			WriteEnable = true;
		}
		else if (mode == 5) { // -e
			EraseEnable = true;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		if (!spi_init()) {
			return -2; // UNABLE TO INIT FTDI
		}

		flashConfig = 0;
		FlashDataInit((unsigned char*)&flashConfig);

		if (((char*)&flashConfig)[0] == ((char*)&flashConfig)[1] &&
			((char*)&flashConfig)[1] == ((char*)&flashConfig)[2] &&
			((char*)&flashConfig)[2] == ((char*)&flashConfig)[3]) {
			return -3; // BAD CONNECTION TO NAND
		}

		if (SFC_init(flashConfig) != OK) {
			return -4; // UNKNOWN NAND
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		if (EraseEnable) {
			for (block_flash = StartBlock; block_flash < block_flash_max; block_flash += Sfc.PageCountInBlock) {
				FlashDataErase(block_flash);
				//PRINT_BLOCKS;
			}
			//PRINT_BLOCKS;
			return 0;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		else if (WriteEnable) {
			FILE* fileW = fopen(FileName.c_str(), "rb");

			if (fileW == NULL) {
				return -8; // NO FILE
			}

			flash_xbox = (unsigned char*)malloc(block_flash_max * 528);

			if (flash_xbox == NULL) {
				return -9; // NO MEMORY
			}

			fseek(fileW, 0L, SEEK_END);
			int File_Size = ftell(fileW);
			fseek(fileW, 0L, SEEK_SET);

			unsigned int File_Blocks = File_Size / block_size;
			unsigned int End_Block = MIN(block_flash_max, File_Blocks);

			memset(flash_xbox, 0, sizeof(flash_xbox));

			fseek(fileW, StartBlock * block_size, SEEK_SET);
			fread((char*)flash_xbox, block_size, End_Block - StartBlock, fileW);

			addr_raw = 0;
			for (block_flash = StartBlock; block_flash < End_Block; ++block_flash) {
				//if ((block_flash % 64) == 0 || block_flash == StartBlock) {
				//	PRINT_BLOCKS;
				//}

				if (PatchECCEnable)
					fixSpare_ECC(&flash_xbox[addr_raw], block_flash);

				FlashDataWrite(&flash_xbox[addr_raw], block_flash, block_size);

				addr_raw += block_size;
			}
			//PRINT_BLOCKS;

			fclose(fileW);
			free(flash_xbox);
			return 0;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		else if (ReadEnable) {
			FILE* fileR = fopen(FileName.c_str(), "wb");
			if (fileR == NULL) {
				return -8; // NO FILE
			}

			flash_xbox = (unsigned char*)malloc(block_flash_max * block_size);

			if (flash_xbox == NULL) {
				return -9; // NO MEMORY
			}

			int byteWrite = fwrite((char*)flash_xbox, 1, 1, fileR);
			if (byteWrite != 1) {
				return -11; // COULDN'T OPEN FILE
			}
			fseek(fileR, 0L, SEEK_SET);

			addr_raw = 0; \
			for (block_flash = StartBlock; block_flash < block_flash_max; ++block_flash) {
				FlashDataRead(&flash_xbox[addr_raw], block_flash, block_size);

				//if ((block_flash % 64) == 0 && (block_flash != 0)) {
				//	PRINT_BLOCKS;
				//}

				addr_raw += block_size;
			}
			if (addr_raw) {
				addr_raw = 0;
				int byteWrite;
				for (block_flash = StartBlock; block_flash < block_flash_max; ++block_flash) {
					do {
						fseek(fileR, addr_raw, SEEK_SET);
						byteWrite = fwrite((char*)&flash_xbox[addr_raw], 1, block_size, fileR);
					} while (byteWrite != block_size);

					addr_raw += block_size;
				}
			}

			//PRINT_BLOCKS;
			fclose(fileR);
			free(flash_xbox);

			return 0;
		}

		return -10; // NO MODE
	}
	__declspec(dllexport) int spiGetBlocks() {
		return block_flash / 32; // Decimal not hex(block_flash/32)
	}
	__declspec(dllexport) int spiGetConfig() {
		return flashConfig; // Decimal not hex
	}
}

int main()
{
	printf("EXE mode unavailable");
	return 1;
}
