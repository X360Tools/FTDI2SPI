
#ifndef WRAPPER_H
#define WRAPPER_H

bool spi_init( void );
void spi_setGPIO( bool GPIO1, bool GPIO2 );
void spi_SetCS( bool ChipSelect );

void AddByteToOutputBuffer( BYTE DataByte, bool bClearOutputBuffer );
void ClearOutputBuffer( void );
void SendBytesToDevice( void );
void DisableSPIChip( void );
void EnableSPIChip( void );
void SetAnswerFast( void );
void AddWriteOutBuffer( DWORD dwNumControlBitsToWrite, unsigned char pWriteControlBuffer[] );
void AddReadOutBuffer( DWORD dwNumDataBitsToRead );
void GetDataFromDevice(unsigned int dwNumDataBitsToRead, unsigned char ReadDataBuffer[] );
void closeDevice();

#endif