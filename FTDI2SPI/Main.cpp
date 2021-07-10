#include "stdafx.h"

#include <windows.h>
#include "wrapper_spi.h"
#include "Flasher.h"
#include "sfc.h"

//#include <process.h>
//#include <stdio.h>

#define APP_NAME "SQUIRTER" 
#define APP_VERSION "0.7 beta" 

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define PRINT_PROGRESS printf( "\r%0.1f%%", ((block_flash-StartBlock)*100)/(float)(block_flash_max-StartBlock) )
                     
#include <iostream>
#include <fstream>
using namespace std;

unsigned int block_size;

int _tmain( int argc, _TCHAR* argv[] )
{	
	unsigned char *flash_xbox;
	unsigned int addr_raw = 0;
	unsigned int StartBlock = 0;
	unsigned int CountBlock = 0;
	unsigned int block_flash = 0;
	unsigned int block_flash_max = 32768;

	bool Wrong_arg = false;
	bool EraseEnable = false;
	bool ReadEnable = false;
	bool WriteEnable = false;
	bool PatchECCEnable = false;
	unsigned int SizeMb = 0;
	char FileName[256] = "";

	printf( "%s ", APP_NAME );
	printf( "v%s\n", APP_VERSION );
	
	for( int i = 0; i < argc && i<=5 ; i++ ) {
		switch( i ) {
			case 0:
				break;
			case 1:
				if( strncmp( argv[i], "ftdi:", 4 ) == 0 ) {
				}
				else {
					Wrong_arg = true;
				}
				break;
			case 2:
				if( strncmp( argv[i], "-r", 2 ) == 0 ) {
					block_size = 0x210;
					ReadEnable = true;
				}
				else if( strncmp( argv[i], "-R", 2 ) == 0 ) {
					block_size = 0x200;
					ReadEnable = true;
				}
				else if( strncmp( argv[i], "-w", 2 ) == 0 ) {
					block_size = 0x210;
					WriteEnable = true;
				}
				else if( strncmp( argv[i], "-e", 2 ) == 0 ) {
					EraseEnable = true;
				}
	
				else if( strncmp( argv[i], "+w", 2 ) == 0 ) {
					PatchECCEnable = true;
					block_size = 0x210;
					WriteEnable = true;				
				}
	
				else {
					Wrong_arg = true;
					break;
				}

				if( isdigit(argv[i][2]) )
					SizeMb = atoi( &argv[i][2] );
				else {
					Wrong_arg = true;					
				}

				block_flash_max = 2048 * SizeMb;
				break;

			case 3:
				strncpy_s( FileName, argv[i], sizeof(FileName) );
				break;

			case 4:
				if( isdigit(argv[i][0]) ) {
					if( sscanf( &argv[i][0], "%x", &StartBlock ) == 0 ) {
						Wrong_arg = true;					
					}
					StartBlock *= 32;
				}
				else {
					Wrong_arg = true;					
				}

				break;
			case 5:
				if( isdigit(argv[i][0]) ) {
					if( sscanf( &argv[i][0], "%x", &CountBlock ) == 0 ) {
						Wrong_arg = true;					
					}
					else {
						block_flash_max = StartBlock + (CountBlock*32);
					}
				}
				else {
					Wrong_arg = true;					
				}
				break;

			default:
				Wrong_arg = true;
				break;
		}

		if( Wrong_arg )
			break;
	}

//////////////////////////////////////////////////////////////

	if( Wrong_arg || ( !EraseEnable && strlen(FileName)==0 ) ) {
		printf( "Useage:\n" );
		printf( "Squirter dev: -r# Filename ->Read\n" );
		printf( "Squirter dev: -w# Filename ->Write\n" );
		printf( "Squirter dev: -e#          ->Erase\n" );
		printf( "\n" );
		printf( "dev: is hardware interface ftdi:\n" );
		printf( "# is nand size (16, 64, 256, 512) in MegaBytes\n" );
		printf( "-r# Reads saving file RAW (with ECC)\n" );
		printf( "-R# Reads saving file without ECC\n" );
		printf( "-w# Writes RAW (with ECC) file\n" );
		printf( "+w# Write RAW (with SPARE) file, init SPARE, block numbers, ECC\n" );
		return -1;		
	}

	printf( "Init USB->FTDI->SPI\n" );
	
	
	if( !spi_init() ) {
		printf( "UNABLE TO INIT FTDI\n" );
		return -2;
	}

	unsigned int flashConfig = 0;
	FlashDataInit( (unsigned char*)&flashConfig );
	 
	printf( "Flash Config: %#08X \n", flashConfig );
	
	if( ((char*)&flashConfig)[0]==((char*)&flashConfig)[1] && 
		((char*)&flashConfig)[1]==((char*)&flashConfig)[2] && 
		((char*)&flashConfig)[2]==((char*)&flashConfig)[3] ) {
			printf( "BAD CONNECTION TO NAND\n" );
			return -3;
	}

	if( SFC_init( flashConfig ) != OK ) {
		printf( "ERROR:UNKNOWN NAND" );
		return -4;
	}

	if( EraseEnable ) {
		for( block_flash=StartBlock; block_flash<block_flash_max; block_flash+=Sfc.PageCountInBlock ) {
			FlashDataErase( block_flash );
			PRINT_PROGRESS;  				
		}
		PRINT_PROGRESS; 
	}
	else if( WriteEnable ) {
		FILE *fileW = fopen( FileName, "rb" );

		if( fileW == NULL ) {
			printf( "File NOT FOUND: %s", FileName );
			return -8;
		}

		flash_xbox = (unsigned char*)malloc( block_flash_max*528 );

		if( flash_xbox==NULL ) {
			printf( "ERROR: NO MEMORY\n" );
			return -9;
		}

		fseek (fileW , 0L , SEEK_END);
		int File_Size = ftell (fileW);
		fseek (fileW , 0L , SEEK_SET);

		unsigned int File_Blocks = File_Size/block_size;
		unsigned int End_Block = MIN(block_flash_max,File_Blocks);

		memset( flash_xbox, 0, sizeof(flash_xbox) );

		fseek (fileW , StartBlock*block_size , SEEK_SET);
		fread( (char*)flash_xbox, block_size, End_Block-StartBlock, fileW );

		printf( "Start Write NAND:\n" );	
		addr_raw = 0;
		for( block_flash=StartBlock; block_flash<End_Block; ++block_flash ) {
			if( (block_flash%64)==0 || block_flash==StartBlock ) {
				printf( "\r%0.1f%%", ((block_flash-StartBlock)*100)/(float)(End_Block-StartBlock) ); 
			}

			if( PatchECCEnable )
				fixSpare_ECC( &flash_xbox[addr_raw], block_flash );

			FlashDataWrite( &flash_xbox[addr_raw], block_flash, block_size );

			addr_raw+=block_size;
		}
		printf( "\r%0.1f%%", ((block_flash-StartBlock)*100)/(float)(End_Block-StartBlock) ); 

		fclose( fileW );
		free( flash_xbox );
		return 0;
	}

//////////////////////////////////////////////////////////////////////////////////////////

	else if( ReadEnable ) {	
		FILE *fileR = fopen( FileName, "wb" );
		if( fileR == NULL ) {
			printf( "File NOT FOUND: %s", FileName );
			return -10;
		}

		flash_xbox = (unsigned char*)malloc( block_flash_max*block_size );

		if( flash_xbox==NULL ) {
			printf( "ERROR: NO MEMORY\n" );
			return -11;
		}

		int byteWrite = fwrite( (char*)flash_xbox, 1, 1, fileR );
		if( byteWrite != 1 ){
			printf( "Can't Write on File: %s", FileName );
			return -11;
		}
		fseek (fileR , 0L , SEEK_SET);

		printf( "Start Read NAND:\n" );
		addr_raw = 0;
		for( block_flash=StartBlock; block_flash<block_flash_max; ++block_flash ) {
			FlashDataRead( &flash_xbox[addr_raw], block_flash, block_size );
			
			if( (block_flash%64)==0 && (block_flash!=0) ) {
				PRINT_PROGRESS; // Do the things to get current block
			}

			addr_raw+=block_size;

		}
		if( addr_raw ) {
			addr_raw = 0;
			int byteWrite;
			for( block_flash=StartBlock; block_flash<block_flash_max; ++block_flash ) {
				do {
					fseek (fileR , addr_raw, SEEK_SET);
					byteWrite = fwrite( (char*)&flash_xbox[addr_raw], 1, block_size, fileR );
				} while( byteWrite != block_size );

				addr_raw+=block_size;
			}
		}

		PRINT_PROGRESS; 
 		fclose( fileR );
		free( flash_xbox );

		return 0;
	}

	return -10;
}
