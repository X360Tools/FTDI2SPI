
#ifndef SFC_H
#define SFC_H

#include <stdint.h>

#define TRUE                                1
#define FALSE                               0
//#define ERROR                               0xFFFFFFFF
#define CANCEL                              0xFFFFFFFE
#define OK                                  1

#define RAW_ENABLE                          1
#define RAW_DISABLE                         0
#define RAW_AUTO                            0xFFFFFFFD

//NAND SIZES
#define NAND_SIZE_16MB                      0x1000000
#define NAND_SIZE_64MB                      0x4000000
#define NAND_SIZE_128MB                     0x8000000
#define NAND_SIZE_256MB                     0x10000000
#define NAND_SIZE_512MB                     0x20000000

//Registers
#define SFCX_CONFIG                            0x00
#define SFCX_STATUS                         0x04
#define SFCX_COMMAND                        0x08
#define SFCX_ADDRESS                        0x0C
#define SFCX_DATA                            0x10
#define SFCX_LOGICAL                         0x14
#define SFCX_PHYSICAL                        0x18
#define SFCX_DPHYSADDR                        0x1C
#define SFCX_MPHYSADDR                        0x20

//Commands for Command Register
#define PAGE_BUF_TO_REG                     0x00 //Read page buffer to Data register
#define REG_TO_PAGE_BUF                     0x01 //Write Data register to page buffer
#define LOG_PAGE_TO_BUF                     0x02 //Read logical page into page buffer
#define PHY_PAGE_TO_BUF                     0x03 //Read physical page into page buffer
#define WRITE_PAGE_TO_PHY                   0x04 //Write page buffer to physical page
#define BLOCK_ERASE                         0x05 //Block Erase
#define DMA_LOG_TO_RAM                      0x06 //DMA logical Flash to main memory
#define DMA_PHY_TO_RAM                      0x07 //DMA physical Flash to main memory
#define DMA_RAM_TO_PHY                      0x08 //DMA main memory to physical Flash
#define UNLOCK_CMD_0                        0x55 //Unlock command 0; Kernel does 0x5500
#define UNLOCK_CMD_1                        0xAA //Unlock command 1; Kernel does 0xAA00

// API Consumers should use these two defines to
// use for creating static buffers at compile time
#define PAGE_SIZE                           0x200   //Max known hardware physical page Size
#define META_SIZE                           0x10
#define BLOCK_SIZE_16K                      0x4000  //
#define BLOCK_SIZE_1K_RAW                   0x420
#define BLOCK_SIZE_16K_RAW                  BLOCK_SIZE_1K_RAW * 16
#define BLOCK_SIZE_128K_RAW                 BLOCK_SIZE_1K_RAW * 128
#define RESERVED_BAD_BLOCK_COUNT            32

//Offsets
#define OFFSET_KV_RAW                       0x4200
#define OFFSET_CB_RAW                       0x8400

//CMD lines
#define CMD_READ_FLASH                      0x00000001
#define CMD_WRITE_FLASH                     0x00000002
#define CMD_KEEP_KV                         0x00000004
#define CMD_KEEP_CONFIG                     0x00000008
#define CMD_ERASE_MU_PARTITION              0x00000010
#define CMD_FULL_READ_WRITE                 0x00000020
#define CMD_WRITE_ONLY_VALID_DATA           0x00000040
#define CMD_FULL_NAND_ERASE                 0x00000080
#define CMD_ERASE_DATA_PARTITION            0x00000100
#define CMD_UPDATE_XELL                     0x00000200  /*NOT TESTED*/
#define CMD_UPDATE_REBOOTER                 0x00000400  /*NOT TESTED*/
#define CMD_UPDATE_REBOOTER_PATCHES         0x00000800  /*NOT TESTED*/
#define CMD_CORRECT_BLOCKNUMBER             0x00001000

typedef struct sfc_struct
{
    uint32_t Initialized;
    uint32_t LargeBlock;
    uint32_t PageSize;
    uint32_t PageSizeRaw;
    uint32_t MetaSize;
    uint32_t PageCountInBlock;
    uint32_t BlockSize;
    uint32_t BlockSizeRaw;
    uint32_t NandSizeMB;
    uint32_t NandSizeDataPartitionBytes;
    uint32_t NandSizeLastDataPartitionBytes;
    uint32_t NandSizeBytesRaw;
    uint32_t NandSizeBytes;
    uint32_t PagesCountInNand;
    uint32_t BlockCountInNandDataPartition;
    uint32_t BlockCountInNand;
    uint32_t OffsetKvRaw;
    uint32_t OffsetCBRaw;
    uint32_t OffsetConfigBlock;
    uint32_t LastBadBlockPos;
    uint32_t CorrectedEcc;
    uint32_t SouthBridgeRevision;
    uint32_t XBOXVersion;
    uint32_t CBVersion;
    uint32_t BadBlockCount;

} sfc_type;

extern sfc_type Sfc;

extern unsigned int SFC_init( unsigned int config);

#endif