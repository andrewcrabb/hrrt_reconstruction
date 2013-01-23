#ifdef ISOTOPEINFO_EXPORTS
#define ISOTOPEINFO_API __declspec(dllexport)
#else
#define ISOTOPEINFO_API __declspec(dllimport)
#endif


enum EN_Isotope {
	Isotope_None=-1,
	BR75=0,	
	C11,
	CU62,
	CU64,
	F18,
	MN52,
	GA68,
	GE68,
	N13,
	O14,
	O15,
	RB82,
	NA22,
	ZN62,
	BR76,
	K38,
	CS137,
	I124,
	Y86
};


typedef struct _tag_isotopeInfo {
	EN_Isotope enIsotope;
	char* isotopeName;
	double halfLife;
	double branchRatio;
} IsotopeInfo, *PIsotopeInfo;



// This class is exported from the isotopeInfo.dll
class ISOTOPEINFO_API CIsotopeInfo {
public:
	static char* getIsotopeName(EN_Isotope isotope);	// NULL for error
	static EN_Isotope getIsotopeEnum(char* isotope);	// -1 for error
	static double halfLife(EN_Isotope isotope);		// 0 for error
	static double halfLife(char* isotope);			// 0 for error
	static double branchRatio(EN_Isotope isotope);	// 0 for error
	static double branchRatio(char* isotope);		// 0 for error
	static int numberOfIsotopes();		// get total number of defined isotopes
	static PIsotopeInfo getAt(int index);		// get an isotope

public:
	CIsotopeInfo();
	CIsotopeInfo(EN_Isotope isotope);
	CIsotopeInfo(char* isotope);

	bool setIsotope(EN_Isotope isotope);	// set isotope after creation
	bool setIsotope(char* isotope);		// set isotope after creation

	char* getIsotopeName();		// NULL for error
	EN_Isotope getIsotopeEnum();	// negative for error

	double halfLife();
	double branchRatio();

private:
	bool ready;
	PIsotopeInfo pIsotopeInfo;
};

