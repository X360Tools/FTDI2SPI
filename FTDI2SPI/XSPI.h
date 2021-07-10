/*! 
*
*/

#ifndef XSPI_H
#define XSPI_H

extern void XSPIInit(void);

extern void XSPIEnterFlashMode(void);
extern void XSPILeaveFlashMode(void);
extern void XSPIPowerUp();
extern void XSPIShutdown();

extern unsigned char XSPI(unsigned char o);
extern void XSPIRead_sync(unsigned char reg, unsigned char Data[]);
extern void XSPIRead_(unsigned char reg, unsigned char Data[]);
extern void XSPIWrite_(unsigned char reg, unsigned char data[] );
extern void XSPIWrite_sync(unsigned char reg, unsigned char data[] );

extern unsigned int XSPIReadWORD_(unsigned char reg);
extern unsigned char XSPIReadBYTE_(unsigned char reg);
extern void XSPIWrite0_(unsigned char reg);
extern void XSPIWriteBYTE_(unsigned char reg, unsigned char d);
extern void XSPIWriteWORD_(unsigned char reg, unsigned int data);

#endif