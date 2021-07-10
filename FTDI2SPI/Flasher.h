/*!
*
*/

#ifndef FLASHER_H
#define FLASHER_H

#include <stdint.h>
#include "sfc.h"

void FlashReadStatus(unsigned char Status[]);
void FlashDataInit(unsigned char FlashConfig[]);
void FlashDataDeInit(void);
void PowerUp(void);
void Shutdown(void);
void Update(void);
void FlashDataErase( unsigned int reg );
void FlashDataRead( unsigned char * pData, unsigned int Arg, unsigned int Words );
void FlashDataWrite( unsigned char Data[], unsigned int Arg, unsigned int Words );
void fixSpare_ECC(unsigned char *blockdata, uint32_t block_num );
void fixSB(unsigned char *blockdata);

#endif