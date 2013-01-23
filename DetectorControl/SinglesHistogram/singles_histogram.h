#include "DHI_Constants.h"

long Singles_Start(long Scanner, long Type, long Rebinner, long Process[MAX_HEADS], long Status[MAX_HEADS]);
long Singles_Histogram(unsigned char *buffer, long NumBytes);
long Time_Histogram(unsigned char *buffer, long NumBytes);
long Singles_Done(long *buffer[MAX_HEADS], long *DataSize, unsigned long *NumEvents, unsigned long *BadAddress, unsigned long *SyncProb, long Status[MAX_HEADS]);
void Singles_Error(long Error, char *Message);
