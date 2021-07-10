
#include "stdafx.h"
#include "sfc.h"

sfc_type Sfc;

unsigned int SFC_init( uint32_t Config)
{
    Sfc.Initialized = FALSE;
    Sfc.LargeBlock = FALSE;
    Sfc.PageSize = PAGE_SIZE;
    Sfc.MetaSize = META_SIZE;
    Sfc.PageSizeRaw = Sfc.PageSize + Sfc.MetaSize;
    Sfc.OffsetKvRaw = OFFSET_KV_RAW;
    Sfc.OffsetCBRaw = OFFSET_CB_RAW;
    Sfc.CorrectedEcc = 0;
    Sfc.BadBlockCount = 0;

    switch((Config >> 17) & 0x03)
    {
        case 0: // Small Block original SFC (pre jasper)
        {
            Sfc.BlockSize = 0x4000; // 16 KB

            switch((Config >> 4) & 0x3)
            {
                case 0: // Unsupported 8MB?
                    return -1;
                case 1: // 16MB
                    Sfc.BlockCountInNandDataPartition = 0x400;
                    Sfc.BlockCountInNand = 0x400;
                    Sfc.NandSizeDataPartitionBytes = Sfc.BlockCountInNandDataPartition << 0xE;
                    Sfc.NandSizeBytes = Sfc.BlockCountInNandDataPartition << 0xE;
                    Sfc.OffsetConfigBlock = 0x3DE;
                    break;

                case 2: // 32MB
                    Sfc.BlockCountInNandDataPartition = 0x800;
                    Sfc.BlockCountInNand = 0x800;
                    Sfc.NandSizeDataPartitionBytes = Sfc.BlockCountInNandDataPartition << 0xE;
                    Sfc.NandSizeBytes = Sfc.BlockCountInNandDataPartition << 0xE;
                    Sfc.OffsetConfigBlock = 0x7BE;
                    break;

                case 3: // 64MB
                    Sfc.BlockCountInNandDataPartition = 0x1000;
                    Sfc.BlockCountInNand = 0x1000;
                    Sfc.NandSizeDataPartitionBytes = Sfc.BlockCountInNandDataPartition << 0xE;
                    Sfc.NandSizeBytes = Sfc.BlockCountInNandDataPartition << 0xE;
                    Sfc.OffsetConfigBlock = 0xF7E;
                    break;
            }
            break;
        }
        case 2: // New SFC/Southbridge: Codename "Panda"? //PATHC
        case 1: // New SFC/Southbridge: Codename "Panda"?
        {
            switch((Config >> 4) & 0x3)
            {
                case 0:
					/* PATCH */
					if(((Config >> 17) & 0x03) == 0x01)
					{
						// Unsupported
						return -1;
					}
					else
					{
						//sfc.meta_type = 1;

						Sfc.BlockSize = 0x4000; // 16 KB
						Sfc.BlockCountInNand = 0x400;
						Sfc.NandSizeBytes = Sfc.BlockCountInNand << 0xE;

						//sfc.blocks_per_lg_block = 8;
						//sfc.size_usable_fs = 0x3E0;
						//sfc.addr_config = 0x0F7C000 - 0x4000;
						Sfc.OffsetConfigBlock = 0x3DE;
						break;
					}

                   //return -1;
			/* PATCH */
				case 1: // Small Block 16MB setup
					if(((Config >> 17) & 0x03) == 0x01)
					{
						// Small block 16MB setup
						Sfc.BlockSize = 0x4000; // 16 KB
						Sfc.BlockCountInNandDataPartition = 0x400;
						Sfc.BlockCountInNand = 0x400;
						Sfc.NandSizeDataPartitionBytes = Sfc.BlockCountInNandDataPartition << 0xE;
						Sfc.NandSizeBytes = Sfc.BlockCountInNandDataPartition << 0xE;
						Sfc.OffsetConfigBlock = 0x3DE;
						break;
					}
					else
					{
						// Small block 64MB setup
						Sfc.BlockSize = 0x4000; // 16 KB
						Sfc.BlockCountInNandDataPartition = 0x1000;
						Sfc.BlockCountInNand = 0x1000;
						Sfc.NandSizeDataPartitionBytes = Sfc.BlockCountInNandDataPartition << 0xE;
						Sfc.NandSizeBytes = Sfc.BlockCountInNandDataPartition << 0xE;
						Sfc.OffsetConfigBlock = 0xF7E;

						break;
					}

                case 2: // Large Block: Current Jasper 256MB and 512MB
                    Sfc.BlockSize = 0x20000; // 128KB
                    Sfc.NandSizeBytes = 0x1 << (((Config >> 19) & 0x3) + ((Config >> 21) & 0xF) + 0x17);
                    Sfc.NandSizeDataPartitionBytes = NAND_SIZE_64MB;
                    Sfc.BlockCountInNandDataPartition = Sfc.NandSizeDataPartitionBytes >> 0x11;
                    Sfc.BlockCountInNand = Sfc.NandSizeBytes >> 0x11;
                    Sfc.LargeBlock = 2;
                    Sfc.OffsetConfigBlock = 0x1DE;
                    break;

                case 3: // Large Block: Future or unknown hardware
                    Sfc.BlockSize = 0x40000; // 256KB
                    Sfc.NandSizeDataPartitionBytes = NAND_SIZE_128MB;
                    Sfc.NandSizeBytes = 0x1 << (((Config >> 19) & 0x3) + ((Config >> 21) & 0xF) + 0x17);
                    Sfc.BlockCountInNandDataPartition = Sfc.NandSizeDataPartitionBytes >> 0x12;
                    Sfc.BlockCountInNand = Sfc.NandSizeBytes >> 0x12;
                    Sfc.LargeBlock = 3;
                    Sfc.OffsetConfigBlock = 0xEE;
                    break;
            }
            break;
        }
        default:
            return -1;
    }

    Sfc.PageCountInBlock = Sfc.BlockSize / Sfc.PageSize;
    Sfc.BlockSizeRaw = Sfc.PageCountInBlock * Sfc.PageSizeRaw;
    Sfc.PagesCountInNand = Sfc.NandSizeDataPartitionBytes / Sfc.PageSize;
    Sfc.BlockCountInNandDataPartition = Sfc.NandSizeDataPartitionBytes / Sfc.BlockSize;
    Sfc.NandSizeBytesRaw = Sfc.BlockSizeRaw * Sfc.BlockCountInNandDataPartition;
    Sfc.NandSizeMB = Sfc.NandSizeDataPartitionBytes >> 20;
    Sfc.LastBadBlockPos = Sfc.BlockCountInNandDataPartition;
    Sfc.NandSizeLastDataPartitionBytes = Sfc.NandSizeDataPartitionBytes - (Sfc.BlockSize * RESERVED_BAD_BLOCK_COUNT);

    //Sfc.XBOXVersion = GetBoardRevisionFromNand();
    Sfc.SouthBridgeRevision = (Config >> 17) & 0x03;
    //Sfc.BadBlockCount = GetBadBlockCount(Sfc.NandSizeDataPartitionBytes);
    Sfc.Initialized = TRUE;

    return OK;
}