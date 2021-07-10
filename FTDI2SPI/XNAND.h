/*!
*
*/

#ifndef XNAND_H
#define XNAND_H

extern unsigned int XNANDReadStart(unsigned int block);
extern void XNANDReadProcess(unsigned char * pData, unsigned char words);

extern unsigned int XNANDErase(unsigned int block);

extern void XNANDWriteStart(void);
extern void XNANDWriteProcess(unsigned char Buffer[], unsigned char Words);
extern unsigned int XNANDWriteExecute(unsigned int block);

#endif