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
unsigned int End_Block = 32768;
bool start_spi = false;
bool stop = false;

extern "C"
{
	__declspec(dllexport) int spi(int mode, int size, char* file, int startblock, int length) {
		unsigned char* flash_xbox;
		unsigned int addr_raw = 0;

		unsigned int StartBlock = startblock * 32;
		unsigned int block_flash_max = 2048 * size;

		if (length > 0) block_flash_max = StartBlock + (length * 32);

		start_spi = false;
		block_flash = 0;
		stop = false;

		bool Wrong_arg = false;
		bool FlashConfig = false;
		bool EraseEnable = false;
		bool ReadEnable = false;
		bool WriteEnable = false;
		bool PatchECCEnable = false;
		string FileName = file;

		if (mode == 0) { // Get Flash Config Only
			FlashConfig = true;
		} else if (mode == 1) { // -r
			block_size = 0x210;
			ReadEnable = true;
		} else if (mode == 2) { // -R
			block_size = 0x200;
			ReadEnable = true;
		} else if (mode == 3) { // -w
			block_size = 0x210;
			WriteEnable = true;
		} else if (mode == 4) { // +w
			PatchECCEnable = true;
			block_size = 0x210;
			WriteEnable = true;
		} else if (mode == 5) { // -e
			EraseEnable = true;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		if (!spi_init()) {
			closeDevice();
			return -2; // UNABLE TO INIT FTDI
		}

		flashConfig = 0;
		FlashDataInit((unsigned char*)&flashConfig);

		if (((char*)&flashConfig)[0] == ((char*)&flashConfig)[1] &&
			((char*)&flashConfig)[1] == ((char*)&flashConfig)[2] &&
			((char*)&flashConfig)[2] == ((char*)&flashConfig)[3]) {
			closeDevice();
			return -3; // BAD CONNECTION TO NAND
		}

		if (SFC_init(flashConfig) != OK) {
			closeDevice();
			return -4; // UNKNOWN NAND
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		if (FlashConfig) {
			closeDevice();
			return 0;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		if (EraseEnable) {
			End_Block = block_flash_max;

			start_spi = true;
			for (block_flash = StartBlock; block_flash < block_flash_max; block_flash += Sfc.PageCountInBlock) {
				if (stop) break;

				FlashDataErase(block_flash);
				//PRINT_BLOCKS;
			}
			//PRINT_BLOCKS;

			closeDevice();

			if (stop) return -1;
			else return 0;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		else if (WriteEnable) {
			FILE* fileW = fopen(FileName.c_str(), "rb");

			if (fileW == NULL) {
				closeDevice();
				return -8; // NO FILE
			}

			flash_xbox = (unsigned char*)malloc(block_flash_max * 528);

			if (flash_xbox == NULL) {
				closeDevice();
				return -9; // NO MEMORY
			}

			fseek(fileW, 0L, SEEK_END);
			int File_Size = ftell(fileW);
			fseek(fileW, 0L, SEEK_SET);

			unsigned int File_Blocks = File_Size / block_size;
			End_Block = MIN(block_flash_max, File_Blocks);

			memset(flash_xbox, 0, sizeof(flash_xbox));

			fseek(fileW, 0, SEEK_SET);
			fread((char*)flash_xbox, block_size, End_Block - StartBlock, fileW);

			start_spi = true;
			addr_raw = 0;
			for (block_flash = StartBlock; block_flash < End_Block; ++block_flash) {
				if (stop) break;

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

			closeDevice();

			if (stop) return -1;
			else return 0;
		}

		//////////////////////////////////////////////////////////////////////////////////////////

		else if (ReadEnable) {
			FILE* fileR = fopen(FileName.c_str(), "wb");
			if (fileR == NULL) {
				closeDevice();
				return -8; // NO FILE
			}

			flash_xbox = (unsigned char*)malloc(block_flash_max * block_size);

			if (flash_xbox == NULL) {
				closeDevice();
				return -9; // NO MEMORY
			}

			int byteWrite = fwrite((char*)flash_xbox, 1, 1, fileR);
			if (byteWrite != 1) {
				closeDevice();
				return -11; // COULDN'T OPEN FILE
			}
			fseek(fileR, 0L, SEEK_SET);

			End_Block = block_flash_max;

			start_spi = true;
			addr_raw = 0;
			for (block_flash = StartBlock; block_flash < block_flash_max; ++block_flash) {
				if (stop) break;

				FlashDataRead(&flash_xbox[addr_raw], block_flash, block_size);

				//if ((block_flash % 64) == 0 && (block_flash != 0)) {
				//	PRINT_BLOCKS;
				//}

				addr_raw += block_size;
			}

			if (stop) {
				closeDevice();
				fclose(fileR);
				free(flash_xbox);
				return -1;
			}

			unsigned int block_file = 0;
			if (addr_raw) {
				addr_raw = 0;
				int byteWrite;
				for (block_file = StartBlock; block_file < block_flash_max; ++block_file) {
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

			closeDevice();
			return 0;
		}

		closeDevice();
		return -10; // NO MODE
	}
	__declspec(dllexport) int spiGetBlocks() {
		if (start_spi) {
			return block_flash / 32; // Decimal not hex
		}
		else {
			return -1;
		}
	}
	__declspec(dllexport) int spiGetConfig() {
		return flashConfig; // Decimal not hex
	}
	__declspec(dllexport) void spiStop() {
		if (start_spi) stop = true;
	}
}

int main()
{
	printf("EXE mode unavailable");
	return 1;
}
