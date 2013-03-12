#include "StdAfx.h"
#include "eventword64bit.h"

// use macro instead of function to increase speed
/*
EventWord64Bit::EventWord64Bit(void)
{
}

EventWord64Bit::~EventWord64Bit(void)
{
}

// return 32 bit tag word
unsigned __int32 EventWord64Bit::GetTagWord(void)
{
	return ((m_EventWord64BitPtr->FirstEW32.EW32Bit & 0x0000FFFF) 
		+((m_EventWord64BitPtr->SecondEW32.EW32Bit & 0x0000FFFF) << 16));
}

// return transaxial head detector index of head A
unsigned __int8 EventWord64Bit::GetAX(void)
{
	return m_EventWord64BitPtr->FirstEW32.EW4Byte.byte0;
}

// return axial head detector index of head A
unsigned __int8 EventWord64Bit::GetAY(void)
{
	return m_EventWord64BitPtr->FirstEW32.EW4Byte.byte1;
}

// return transaxial head detector index of head B
unsigned __int8 EventWord64Bit::GetBX(void)
{
	return m_EventWord64BitPtr->SecondEW32.EW4Byte.byte0;
}

// return axial head detector index of head B
unsigned __int8 EventWord64Bit::GetBY(void)
{
	return m_EventWord64BitPtr->SecondEW32.EW4Byte.byte1;
}

// return module pair
unsigned __int8 EventWord64Bit::GetXE(void)
{
	return ((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte2 & 0x07)
		+ ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte2 & 0x07) << 3));
}

// return energy window of head A
unsigned __int8 EventWord64Bit::GetAE(void)
{
	return ((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte2 & 0x38) >> 3);
}

// return energy window of head B
unsigned __int8 EventWord64Bit::GetBE(void)
{
	return ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte2 & 0x38) >> 3);
}

// return DOI of head A
unsigned __int8 EventWord64Bit::GetAI(void)
{
	return (((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x01)<< 2)
		+ ((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte2 & 0xC0) >>6));
}

// return DOI of head B
unsigned __int8 EventWord64Bit::GetBI(void)
{
	return (((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x01)<< 2)
		+ ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte2 & 0xC0) >>6));
}

// return time of flight
unsigned __int8 EventWord64Bit::GetTF(void)
{
	return (((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x0E)>>1)
		+ ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x0E)<<2));
}

BOOL EventWord64Bit::IsPrompt(void)
{
	return (m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x40);
}

BOOL EventWord64Bit::IsTagWord(void)
{
	if ((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x40))
		return TRUE;
	else
        return FALSE;
}

BOOL EventWord64Bit::IsSynchronized(void)
{
	if (((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x80) == 0) &&
		((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x80) ==1))
		return TRUE;
	else
		return FALSE;
}

*/