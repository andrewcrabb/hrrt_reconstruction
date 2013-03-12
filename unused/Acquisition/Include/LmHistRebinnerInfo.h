#ifdef UseWithMedCom

#ifndef MedComLmHistRebinnerInfo_h
#define MedComLmHistRebinnerInfo_h
// info about rebinner configuration
class MedComRebinner_info {
public:
	MedComRebinner_info(void) {};
DECLARE_MSC(MedComRebinner_info)
	ULONG lut;
	ULONG span;
	ULONG ringDifference;
	
};
#ifndef NoImpl
IMPLEMENT_MSC(MedComRebinner_info, G(lut) G(span) G(ringDifference))
#endif
#endif

#else

#ifndef LmHistRebinnerInfo_h
#define LmHistRebinnerInfo_h
// info about rebinner configuration
class Rebinner_info {
public:
	Rebinner_info(void) {};
	
	ULONG lut;
	ULONG span;
	ULONG ringDifference;
};
#endif

#endif
