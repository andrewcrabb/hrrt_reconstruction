// SinglesAcqDLL.h - include file to export DLL entry point
//

#ifndef SINGLESACQDLLAPI
#define SINGLESACQDLLAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

SINGLESACQDLLAPI int AcquireSingles (int PresetTime, int RebinnerMode, int TimeOut) ;
SINGLESACQDLLAPI int StopAcq () ;
SINGLESACQDLLAPI void SetSAcqEnv (char* Filename, int verbose, int debug, int lut) ;

#ifdef __cplusplus
}
#endif

