
#include <math.h>

float GetDTC(int LLD, long SinPerBlock)
{
	double EnergyLevel;
	
	if (LLD == 250)
		EnergyLevel = 6.9378e-6;
	if (LLD > 250  && LLD <300)
		EnergyLevel = (((double)LLD - 250.0)/50.0) * (7.0677e-6 - 6.9378e-6) + 6.9378e-6;
	if (LLD == 300)
		EnergyLevel = 7.0677e-6;
	if (LLD > 300  && LLD <350)
		EnergyLevel = (((double)LLD - 300.0)/50.0) * (7.7004e-6 - 7.0677e-6 ) + 7.0677e-6;
	if (LLD == 350)
		EnergyLevel = 7.7004e-6;
	if (LLD > 350  && LLD <400)
		EnergyLevel = (((double)LLD - 350.0)/50.0) * (8.94e-6 - 7.7004e-6) + 7.7004e-6;
	if (LLD >= 400)
		EnergyLevel = 8.94e-6;


	return((float)exp(EnergyLevel*(double)SinPerBlock));
	
}
