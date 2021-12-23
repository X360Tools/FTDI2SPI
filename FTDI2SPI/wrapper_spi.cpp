#include "stdafx.h"
#include <windows.h>

#include "FTCSPI.h"
#include "FTD2XX.H"

#define MAX_NUM_BYTES_USB_WRITE 4096

#define MAX_READ_DATA_WORDS_BUFFER_SIZE 65536    // 64k bytes
#define MAX_FREQ_CLOCK_DIVISOR			0

#define CHIP_SELECT_PIN '\x08'

const BYTE SET_LOW_BYTE_DATA_BITS_CMD = '\x80';
const BYTE SET_HIGH_BYTE_DATA_BITS_CMD = '\x82';
const BYTE SEND_ANSWER_BACK_IMMEDIATELY_CMD = '\x87';

const BYTE CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD = '\x19';
const BYTE CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD = '\x1B';
const BYTE CLK_DATA_BYTES_IN_ON_NEG_CLK_LSB_FIRST_CMD = '\x2D';
const BYTE CLK_DATA_BITS_IN_ON_NEG_CLK_LSB_FIRST_CMD = '\x2F';


typedef WORD ReadDataWordBuffer[MAX_READ_DATA_WORDS_BUFFER_SIZE];
typedef ReadDataWordBuffer *PReadDataWordBuffer;

FTC_HANDLE ftHandle = 0;  
FTC_STATUS Status = FTC_SUCCESS;

FTC_INIT_CONDITION WriteStartCondition;
FTC_INIT_CONDITION ReadStartCondition;
  
WriteControlByteBuffer WriteControlBuffer;
WriteDataByteBuffer WriteDataBuffer;
DWORD dwNumDataBytesReturned = 0;
FTC_WAIT_DATA_WRITE WaitDataWriteComplete;
FTH_HIGHER_OUTPUT_PINS HighPinsWriteActiveStates;

FTC_CHIP_SELECT_PINS ChipSelectsDisableStates;
FTH_INPUT_OUTPUT_PINS HighInputOutputPins;
  
bool spi_init( void )
{
  DWORD dwNumHiSpeedDevices = 0;
  char szDeviceName[100];
  char szChannel[5];
  DWORD dwLocationID = 0;
  DWORD dwHiSpeedDeviceType = 0;
  DWORD dwHiSpeedDeviceIndex = 0;
  char szDeviceDetails[150];
  BYTE timerValue = 0;
  DWORD dwClockFrequencyHz = 0;
  bool bPerformCommandSequence = false;
//  FTH_LOW_HIGH_PINS HighPinsInputData;
  DWORD dwNumDataBytesToWrite = 0;
  //FTC_CLOSE_FINAL_STATE_PINS CloseFinalStatePinsData;
  DWORD dwDataWordInitValue = 0;
  DWORD dwDataWordValue = 0;
  DWORD dwWriteDataWordAddress = 0;
  DWORD dwControlLocAddress1 = 0;
  DWORD dwControlLocAddress2 = 0;
  DWORD dwReadDataWordAddress = 0;
  WORD dwReadWordValue = 0;
  //ReadDataWordBuffer ReadWordData;
  DWORD dwDataWordWritten = 0;
  DWORD dwCharCntr = 0;
//ReadDataByteBuffer ReadDataBuffer;
  DWORD dwReadDataIndex = 0;
//ReadCmdSequenceDataByteBuffer ReadCmdSequenceDataBuffer;
  int MsgBoxKeyPressed = 0;
  DWORD dwLoopCntr = 0;

  char szDllVersion[10];
//  char szTitleErrorMessage[100];
//  char szStatusErrorMessage[100];
  //char szErrorMessage[200];
  char szMismatchMessage[100];

  for (dwCharCntr = 0; (dwCharCntr < 100); dwCharCntr++)
    szMismatchMessage[dwCharCntr] = '\0';

  Status = SPI_GetDllVersion(szDllVersion, 10);
//  MessageBox(NULL, szDllVersion, "FTCSPI DLL Version", MB_OK);

  Status = SPI_GetNumHiSpeedDevices(&dwNumHiSpeedDevices);

  if ((Status == FTC_SUCCESS) && (dwNumHiSpeedDevices > 0))
  {
    do
    {
      Status = SPI_GetHiSpeedDeviceNameLocIDChannel(dwHiSpeedDeviceIndex, szDeviceName, 100, &dwLocationID, szChannel, 5, &dwHiSpeedDeviceType);

      dwHiSpeedDeviceIndex = dwHiSpeedDeviceIndex + 1;
    }
    while ((Status == FTC_SUCCESS) && (dwHiSpeedDeviceIndex < dwNumHiSpeedDevices) && (strcmp(szChannel, "B") != 0));

    if (Status == FTC_SUCCESS)
    {
      if (strcmp(szChannel, "B") != 0)
        Status = FTC_DEVICE_IN_USE;
    }

    if (Status == FTC_SUCCESS) {
      Status = SPI_OpenHiSpeedDevice(szDeviceName, dwLocationID, szChannel, &ftHandle);

      if (Status == FTC_SUCCESS) {
        Status = SPI_GetHiSpeedDeviceType(ftHandle, &dwHiSpeedDeviceType);

        if (Status == FTC_SUCCESS) {
          strcpy_s(szDeviceDetails, "Type = ");

          if (dwHiSpeedDeviceType == FT4232H_DEVICE_TYPE)
            strcat_s(szDeviceDetails, "FT4232H");
          else if (dwHiSpeedDeviceType == FT2232H_DEVICE_TYPE)
            strcat_s(szDeviceDetails, "FT2232H");

          strcat_s(szDeviceDetails, ", Name = ");
          strcat_s(szDeviceDetails, szDeviceName);

          //MessageBox(NULL, szDeviceDetails, "Hi Speed Device", MB_OK);
        }
      }
    }
  }

  if ((Status == FTC_SUCCESS) && (ftHandle != 0))
  {
    Status = SPI_InitDevice(ftHandle, MAX_FREQ_CLOCK_DIVISOR); //65536

    if (Status == FTC_SUCCESS) {
      if ((Status = SPI_GetDeviceLatencyTimer(ftHandle, &timerValue)) == FTC_SUCCESS) {
        if ((Status = SPI_SetDeviceLatencyTimer(ftHandle, 50)) == FTC_SUCCESS) {
          Status = SPI_GetDeviceLatencyTimer(ftHandle, &timerValue);

          Status = SPI_SetDeviceLatencyTimer(ftHandle, 1);

          Status = SPI_GetDeviceLatencyTimer(ftHandle, &timerValue);
        }
      }
    }

    if (Status == FTC_SUCCESS)
    {
      if ((Status = SPI_GetHiSpeedDeviceClock(0, &dwClockFrequencyHz)) == FTC_SUCCESS)
      {
        if ((Status = SPI_TurnOnDivideByFiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
        {
          Status = SPI_GetHiSpeedDeviceClock(0, &dwClockFrequencyHz);

          if ((Status = SPI_SetClock(ftHandle, MAX_FREQ_CLOCK_DIVISOR, &dwClockFrequencyHz)) == FTC_SUCCESS)
          {
            if ((Status = SPI_TurnOffDivideByFiveClockingHiSpeedDevice(ftHandle)) == FTC_SUCCESS)
              Status = SPI_SetClock(ftHandle, MAX_FREQ_CLOCK_DIVISOR, &dwClockFrequencyHz);
          }
        }
      }
    }

    if (Status == FTC_SUCCESS)
    {
//      Status = SPI_SetLoopback(ftHandle, false);

      if (Status == FTC_SUCCESS)
      {
        bPerformCommandSequence = false;

        if (bPerformCommandSequence == true)
        {
            if (Status == FTC_SUCCESS)
              Status = SPI_ClearDeviceCmdSequence(ftHandle);
        }

        if (Status == FTC_SUCCESS)
        {
          // Must set the chip select disable states for all the SPI devices connected to a FT2232H hi-speed dual
          // device or FT4332H hi-speed quad device
          ChipSelectsDisableStates.bADBUS3ChipSelectPinState = true;
          ChipSelectsDisableStates.bADBUS4GPIOL1PinState = false;
          ChipSelectsDisableStates.bADBUS5GPIOL2PinState = false;
          ChipSelectsDisableStates.bADBUS6GPIOL3PinState = false;
          ChipSelectsDisableStates.bADBUS7GPIOL4PinState = false;

          HighInputOutputPins.bPin1InputOutputState = true;
          HighInputOutputPins.bPin1LowHighState = false;
          HighInputOutputPins.bPin2InputOutputState = true;
          HighInputOutputPins.bPin2LowHighState = true;
          HighInputOutputPins.bPin3InputOutputState = false;
          HighInputOutputPins.bPin3LowHighState = false;
          HighInputOutputPins.bPin4InputOutputState = false;
          HighInputOutputPins.bPin4LowHighState = false;

          HighInputOutputPins.bPin5InputOutputState = false;
          HighInputOutputPins.bPin5LowHighState = false;
          HighInputOutputPins.bPin6InputOutputState = false;
          HighInputOutputPins.bPin6LowHighState = false;
          HighInputOutputPins.bPin7InputOutputState = false;
          HighInputOutputPins.bPin7LowHighState = false;
          HighInputOutputPins.bPin8InputOutputState = false;
          HighInputOutputPins.bPin8LowHighState = false;

          Status = SPI_SetHiSpeedDeviceGPIOs(ftHandle, &ChipSelectsDisableStates, &HighInputOutputPins); //NULL
/*
          if (Status == FTC_SUCCESS)
          {
            Sleep(200);

            Status = SPI_GetHiSpeedDeviceGPIOs(ftHandle, &HighPinsInputData); //NULL);

            Sleep(200);
          }
*/
        }        
      }
    }
  }
/*
  if (ftHandle != 0) 
  {
    //CloseFinalStatePinsData.bTCKPinState = true;
    //CloseFinalStatePinsData.bTCKPinActiveState = true;
    //CloseFinalStatePinsData.bTDIPinState = true;
    //CloseFinalStatePinsData.bTDIPinActiveState = false;
    //CloseFinalStatePinsData.bTMSPinState = true;
    //CloseFinalStatePinsData.bTMSPinActiveState = false;

    //Status = SPI_CloseDevice(ftHandle, &CloseFinalStatePinsData);

    SPI_Close(ftHandle);
    ftHandle = 0;
  }

  if ((Status != FTC_SUCCESS) || (dwNumHiSpeedDevices == 0) || (strlen(szMismatchMessage) > 0))
  {
    if (Status != FTC_SUCCESS)
    {
      sprintf_s(szErrorMessage, "Status Code(%u) - ", Status);

      Status = SPI_GetErrorCodeString("EN", Status, szStatusErrorMessage, 100);

      strcat_s(szErrorMessage, szStatusErrorMessage);

//      MessageBox(NULL, szErrorMessage, "FTCSPI DLL Error Status Message", MB_OK);
    } else {
      if (dwNumHiSpeedDevices == 0)
        strcpy_s(szErrorMessage, "There are no devices connected.");
      else
        strcpy_s(szErrorMessage, szMismatchMessage);

//      MessageBox(NULL, szErrorMessage, "FTCSPI DLL Error Message", MB_OK);
    }
  }
  else
  {
    Status = SPI_GetDllVersion(szDllVersion, 10);

    //if (Status == FTC_SUCCESS)
      //MessageBox(NULL, szDllVersion, "SPI DLL Version", MB_OK);
    //else
    //{
      //Status = I2C_GetErrorCodeString("EN", Status, szErrorMessage, 100);

      //MessageBox(NULL, szErrorMessage, "SPI Error Status Message", MB_OK);
    //}

    strcpy_s(szErrorMessage, "Passed.");

//    MessageBox(NULL, szErrorMessage, "FTCSPI DLL Message", MB_OK);
  }
  */

	// Set the 8 general purpose higher input/output pins
	HighPinsWriteActiveStates.bPin1ActiveState = true;
	HighPinsWriteActiveStates.bPin1State = true;
	HighPinsWriteActiveStates.bPin2ActiveState = true;
	HighPinsWriteActiveStates.bPin2State = true;
	HighPinsWriteActiveStates.bPin3ActiveState = false;
	HighPinsWriteActiveStates.bPin3State = false;
	HighPinsWriteActiveStates.bPin4ActiveState = false;
	HighPinsWriteActiveStates.bPin4State = false;
	HighPinsWriteActiveStates.bPin5ActiveState = false;
	HighPinsWriteActiveStates.bPin5State = false;
	HighPinsWriteActiveStates.bPin6ActiveState = false;
	HighPinsWriteActiveStates.bPin6State = false;
	HighPinsWriteActiveStates.bPin7ActiveState = false;
	HighPinsWriteActiveStates.bPin7State = false;
	HighPinsWriteActiveStates.bPin8ActiveState = false;
	HighPinsWriteActiveStates.bPin8State = false;

	return (Status == FTC_SUCCESS && dwNumHiSpeedDevices > 0 ) ? true : false;

}

BYTE byOutputBuffer[65535];
BYTE dwLowPinsValue = 0;
DWORD dwNumBytesToSend = 0; // Index to the output buffer
DWORD dwNumBytesSent = 0; // Count of actual bytes sent - used with FT_Write
DWORD dwNumBytesToRead = 0; // Number of bytes available to read

void SendBytesToDevice( void )
{
  FTC_STATUS Status = FTC_SUCCESS;
  DWORD dwNumDataBytesToSend = 0;
  DWORD dwNumBytesSent = 0;
  DWORD dwTotalNumBytesSent = 0;

  if (dwNumBytesToSend > MAX_NUM_BYTES_USB_WRITE)
  {
    do
    {
      // 25/08/05 - Can only use 4096 byte block as Windows 2000 Professional does not allow you to alter the USB buffer size
      // 25/08/05 - Windows 2000 Professional always sets the USB buffer size to 4K ie 4096
      if ((dwTotalNumBytesSent + MAX_NUM_BYTES_USB_WRITE) <= dwNumBytesToSend)
        dwNumDataBytesToSend = MAX_NUM_BYTES_USB_WRITE;
      else
        dwNumDataBytesToSend = (dwNumBytesToSend - dwTotalNumBytesSent);

      // This function sends data to a FT2232C dual type device. The dwNumBytesToSend variable specifies the number of
      // bytes in the output buffer to be sent to a FT2232C dual type device. The dwNumBytesSent variable contains
      // the actual number of bytes sent to a FT2232C dual type device.
      Status = FT_Write((FT_HANDLE)ftHandle, &byOutputBuffer[dwTotalNumBytesSent], dwNumDataBytesToSend, &dwNumBytesSent);

      dwTotalNumBytesSent = dwTotalNumBytesSent + dwNumBytesSent;
    }
    while ((dwTotalNumBytesSent < dwNumBytesToSend) && (Status == FTC_SUCCESS)); 
  }
  else
  {
    // This function sends data to a FT2232C dual type device. The dwNumBytesToSend variable specifies the number of
    // bytes in the output buffer to be sent to a FT2232C dual type device. The dwNumBytesSent variable contains
    // the actual number of bytes sent to a FT2232C dual type device.
    Status = FT_Write((FT_HANDLE)ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);
  }

  dwNumBytesToSend = 0;

}

void ClearOutputBuffer(void)
{
  dwNumBytesToSend = 0;
}

void AddByteToOutputBuffer( BYTE DataByte, bool bClearOutputBuffer )
{
	if( bClearOutputBuffer )
		dwNumBytesToSend = 0;

	byOutputBuffer[dwNumBytesToSend++] = DataByte;
}

void SetAnswerFast( void )
{
	AddByteToOutputBuffer( SEND_ANSWER_BACK_IMMEDIATELY_CMD, false );	
}

void GetDataFromDevice(unsigned int dwNumBytesToRead, unsigned char ReadDataBuffer[] )
{
	DWORD dwNumBytesRead = 0;
//	DWORD dwNumBytesToRead = dwNumDataBitsToRead/8;	
	DWORD dwBytesReadIndex = 0;

	int try_count = 10;
	
	do {
		FT_Read((FT_HANDLE)ftHandle, &ReadDataBuffer[dwBytesReadIndex], dwNumBytesToRead, &dwNumBytesRead);
		dwBytesReadIndex += dwNumBytesRead;
		dwNumBytesToRead -= dwNumBytesRead;
	} while( dwNumBytesToRead > 0 && try_count-- > 0 );

	if( try_count <= 0 )
		throw "ERROR: NO DATA FROM DEVICE";
	
	
}
void DisableSPIChip( void )
{
	AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, false);
	dwLowPinsValue = (dwLowPinsValue | CHIP_SELECT_PIN); // set CS to high
	// set SK, DO, CS and GPIOL1-4 as output, set D1 as input
	AddByteToOutputBuffer(dwLowPinsValue, FALSE);
	AddByteToOutputBuffer('\xFB', false);
}

void EnableSPIChip( void )
{
	AddByteToOutputBuffer(SET_LOW_BYTE_DATA_BITS_CMD, false);
	dwLowPinsValue = (dwLowPinsValue & ~CHIP_SELECT_PIN); // set CS to low
	// set SK, DO, CS and GPIOL1-4 as output, set D1 as input
	AddByteToOutputBuffer(dwLowPinsValue, FALSE);
	AddByteToOutputBuffer('\xFB', false);
}

void AddWriteOutBuffer( DWORD dwNumControlBitsToWrite, unsigned char pWriteControlBuffer[] )
{
  DWORD dwModNumControlBitsToWrite = 0;
  DWORD dwControlBufferIndex = 0;
  DWORD dwNumControlBytes = 0;
  DWORD dwNumRemainingControlBits = 0;
  DWORD dwModNumDataBitsToWrite = 0;
  DWORD dwDataBufferIndex = 0;
  DWORD dwNumDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;

  // kra - 040608, added test for number of control bits to write, because for SPI only, the number of control
  // bits to write can be 0 on some SPI devices, before a read operation is performed
  if (dwNumControlBitsToWrite > 1)
  {
    // adjust for bit count of 1 less than no of bits
    dwModNumControlBitsToWrite = (dwNumControlBitsToWrite - 1);

    // Number of control bytes is greater than 0, only if the minimum number of control bits is 8
    dwNumControlBytes = (dwModNumControlBitsToWrite / 8);

    if (dwNumControlBytes > 0)
    {
      // Number of whole bytes
      dwNumControlBytes = (dwNumControlBytes - 1);

      // clk data bytes out
      AddByteToOutputBuffer(CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD, FALSE);
      AddByteToOutputBuffer((dwNumControlBytes & '\xFF'), FALSE);
      AddByteToOutputBuffer(((dwNumControlBytes / 256) & '\xFF'), FALSE);

      // now add the data bytes to go out
      do
      {
        AddByteToOutputBuffer( pWriteControlBuffer[dwControlBufferIndex], FALSE);
        dwControlBufferIndex = (dwControlBufferIndex + 1);
      }
      while (dwControlBufferIndex < (dwNumControlBytes + 1));
    }

    dwNumRemainingControlBits = (dwModNumControlBitsToWrite % 8);

    // do remaining bits
    if (dwNumRemainingControlBits > 0)
    {
      // clk data bits out
      //*lpdwDataWriteBytesCommand = CLK_DATA_BYTES_OUT_ON_NEG_CLK_LSB_FIRST_CMD;
      //*lpdwDataWriteBitsCommand = CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD;
      AddByteToOutputBuffer(CLK_DATA_BITS_OUT_ON_NEG_CLK_LSB_FIRST_CMD, FALSE);
      AddByteToOutputBuffer((dwNumRemainingControlBits & '\xFF'), FALSE);
      AddByteToOutputBuffer( pWriteControlBuffer[dwControlBufferIndex], FALSE);
    }
  }
}


void AddReadOutBuffer( DWORD dwNumDataBitsToRead )
{
  DWORD dwModNumBitsToRead = 0;
  DWORD dwNumDataBytes = 0;
  DWORD dwNumRemainingDataBits = 0;

  // adjust for bit count of 1 less than no of bits
  dwModNumBitsToRead = (dwNumDataBitsToRead - 1);

  dwNumDataBytes = (dwModNumBitsToRead / 8);

  if (dwNumDataBytes > 0)
  {
    // Number of whole bytes
    dwNumDataBytes = (dwNumDataBytes - 1);

    // clk data bytes out
    AddByteToOutputBuffer(CLK_DATA_BYTES_IN_ON_NEG_CLK_LSB_FIRST_CMD, FALSE);
    AddByteToOutputBuffer((dwNumDataBytes & '\xFF'), FALSE);
    AddByteToOutputBuffer(((dwNumDataBytes / 256) & '\xFF'), FALSE);
  }

  // number of remaining bits
  dwNumRemainingDataBits = (dwModNumBitsToRead % 8);

  if (dwNumRemainingDataBits > 0)
  {
    // clk data bits out
    AddByteToOutputBuffer(CLK_DATA_BITS_IN_ON_NEG_CLK_LSB_FIRST_CMD, FALSE);
    AddByteToOutputBuffer((dwNumRemainingDataBits & '\xFF'), FALSE);
  }
}

void spi_SetCS( bool ChipSelect )
{
	dwNumBytesToSend = 0; // Index to the output buffer
	dwNumBytesSent = 0; // Count of actual bytes sent - used with FT_Write
	dwNumBytesToRead = 0; // Number of bytes available to read

	byOutputBuffer[dwNumBytesToSend++] = 0x80;
	dwLowPinsValue &= ~0x08;
	dwLowPinsValue |= ChipSelect ? 0x08 : 0x00;
	byOutputBuffer[dwNumBytesToSend++] = dwLowPinsValue;
	byOutputBuffer[dwNumBytesToSend++] = 0x3E; // byDirection
	FT_STATUS ftStatus = FT_Write( (FT_HANDLE)ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);
//	if(ftStatus == FT_OK)
//		while(dwNumBytesSent != dwNumBytesToSend)
//			printf("Sending byte %d\n", dwNumBytesSent);

	dwNumBytesToSend = 0;
	dwNumBytesToRead = 0;	
}

void spi_setGPIO( bool XXLo, bool EJLo )
{
	dwNumBytesToSend = 0; // Index to the output buffer
	dwNumBytesSent = 0; // Count of actual bytes sent - used with FT_Write
	dwNumBytesToRead = 0; // Number of bytes available to read

	byOutputBuffer[dwNumBytesToSend++] = 0x80;
	dwLowPinsValue &= ~0x30;
	dwLowPinsValue |= (XXLo ? 0x10 : 0x00) | (EJLo ? 0x20 : 0x00);
	byOutputBuffer[dwNumBytesToSend++] = dwLowPinsValue; 
	byOutputBuffer[dwNumBytesToSend++] = 0x3E; // byDirection
	FT_STATUS ftStatus = FT_Write( (FT_HANDLE)ftHandle, byOutputBuffer, dwNumBytesToSend, &dwNumBytesSent);
//	if(ftStatus == FT_OK)
//		while(dwNumBytesSent != dwNumBytesToSend)
//			printf("Sending byte %d\n", dwNumBytesSent);

	dwNumBytesToSend = 0;
	dwNumBytesToRead = 0;	
}

void closeDevice() {
    if (ftHandle != 0) {
        FTC_CLOSE_FINAL_STATE_PINS CloseFinalStatePinsData;
        CloseFinalStatePinsData.bTCKPinState = true;
        CloseFinalStatePinsData.bTCKPinActiveState = true;
        CloseFinalStatePinsData.bTDIPinState = true;
        CloseFinalStatePinsData.bTDIPinActiveState = false;
        CloseFinalStatePinsData.bTMSPinState = true;
        CloseFinalStatePinsData.bTMSPinActiveState = false;

        Status = SPI_CloseDevice(ftHandle, &CloseFinalStatePinsData);

        SPI_Close(ftHandle);
        ftHandle = 0;
    }
}