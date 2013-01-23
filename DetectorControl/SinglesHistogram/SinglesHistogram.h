#include "DHI_Constants.h"

__declspec(dllexport) long Singles_Start(long Scanner, long Type, long Rebinner, unsigned long MaxKEvents, long Process[MAX_HEADS], long *buffer[MAX_HEADS], long Status[MAX_HEADS]);
__declspec(dllexport) long Singles_Histogram(unsigned char *buffer, long NumBytes);
__declspec(dllexport) long Coinc_Histogram(unsigned char *buffer, long NumBytes);
__declspec(dllexport) long Singles_Done(unsigned long *NumKEvents, unsigned long *BadAddress, unsigned long *SyncProb, long Status[MAX_HEADS]);
__declspec(dllexport) long Histogram_Buffer(unsigned char *buffer, long NumBytes, long Flag);
__declspec(dllexport) double CountPercent();

