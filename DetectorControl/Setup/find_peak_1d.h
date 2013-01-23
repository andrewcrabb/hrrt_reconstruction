void Smooth(long range, long passes, long in[256], double out[256]);
long Find_Peak_1d (double dData[256], long *lPeaks);
long Find_Valley_1d (double dData[256], long *lValleys);
long Peak_Most_Counts (double dData[256]);
long Determine_Gain(long OldGain, long Peak, double Current[256], double Master[256]);
double peak_analyze(long *data, long length, double *centroid);
