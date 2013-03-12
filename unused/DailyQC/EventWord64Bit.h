#pragma once

// Class to get information from 64 bit event word
// Created by Dongming Hu on 05/12/2005
// Revision history
// 05/12/05  HDM first entry
//

// use macro instead of memeber function
//class EventWord64Bit
//{
//public:
	struct EventWord32BitStruct
	{
		unsigned __int8 byte0;
		unsigned __int8 byte1;
		unsigned __int8 byte2;
		unsigned __int8 byte3;
	} ;//* m_EventWord32BitPtr;

	union EventWord32BitUnion
	{
		unsigned __int32 EW32Bit;
		EventWord32BitStruct EW4Byte;
	};
	struct EventWord64BitStruct
	{
		EventWord32BitUnion FirstEW32;
		EventWord32BitUnion SecondEW32;

	};// * m_EventWord64BitPtr;
/*
public:
	EventWord64Bit(void);
	~EventWord64Bit(void);
	// return 32 bit tag word
	unsigned __int32 GetTagWord(void);
	// return transaxial head detector index of head A
	unsigned __int8 GetAX(void);
	// return axial head detector index of head A
	unsigned __int8 GetAY(void);
	// return transaxial head detector index of head B
	unsigned __int8 GetBX(void);
	// return axial head detector index of head B
	unsigned __int8 GetBY(void);
	// return module pair
	unsigned __int8 GetXE(void);
	// return energy window of head A
	unsigned __int8 GetAE(void);
	// return energy window of head B
	unsigned __int8 GetBE(void);
	// return DOI of head A
	unsigned __int8 GetAI(void);
	// return DOI of head B
	unsigned __int8 GetBI(void);
	// return time of flight
	unsigned __int8 GetTF(void);
	BOOL IsPrompt(void);
	BOOL IsTagWord(void);
	BOOL IsSynchronized(void);
};
*/

// return 32 bit tag word
#define GetTagWord(m_EventWord64BitPtr) \
( \
	((m_EventWord64BitPtr->FirstEW32.EW32Bit & 0x0000FFFF) \
		+((m_EventWord64BitPtr->SecondEW32.EW32Bit & 0x0000FFFF) << 16))\
)
// return transaxial head detector index of head A
#define GetAX(m_EventWord64BitPtr) \
(\
	m_EventWord64BitPtr->FirstEW32.EW4Byte.byte0 \
)

// return axial head detector index of head A
#define GetAY(m_EventWord64BitPtr) \
(\
	m_EventWord64BitPtr->FirstEW32.EW4Byte.byte1 \
)

// return transaxial head detector index of head B
#define GetBX(m_EventWord64BitPtr) \
( \
	m_EventWord64BitPtr->SecondEW32.EW4Byte.byte0 \
)

// return axial head detector index of head B
#define GetBY(m_EventWord64BitPtr) \
( \
	m_EventWord64BitPtr->SecondEW32.EW4Byte.byte1 \
)

// return module pair
#define GetXE(m_EventWord64BitPtr) \
( \
	((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte2 & 0x07) \
		+ ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte2 & 0x07) << 3)) \
)

// return energy window of head A
#define GetAE(m_EventWord64BitPtr) \
( \
	((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte2 & 0x38) >> 3) \
)

// return energy window of head B
#define GetBE(m_EventWord64BitPtr) \
( \
	((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte2 & 0x38) >> 3) \
)

// return DOI of head A
#define GetAI(m_EventWord64BitPtr)\
( \
	(((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x01)<< 2) \
		+ ((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte2 & 0xC0) >>6)) \
)

// return DOI of head B
#define GetBI(m_EventWord64BitPtr) \
( \
	(((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x01)<< 2) \
		+ ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte2 & 0xC0) >>6)) \
)

// return time of flight
#define GetTF(m_EventWord64BitPtr) \
( \
	(((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x0E)>>1) \
		+ ((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x0E)<<2)) \
)

#define IsPrompt(m_EventWord64BitPtr) \
( \
	(m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x40) \
)

#define IsTagWord(m_EventWord64BitPtr) \
( \
	((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x40)) \
)

#define IsSynchronized(m_EventWord64BitPtr) \
( \
	(((m_EventWord64BitPtr->FirstEW32.EW4Byte.byte3 & 0x80) == 0) && \
		((m_EventWord64BitPtr->SecondEW32.EW4Byte.byte3 & 0x80) !=0)) \
)
