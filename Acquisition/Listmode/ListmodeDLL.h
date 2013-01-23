// ListmodeDLL.h - include file to export DLL entry points
//

#ifndef LISTMODEDLLAPI
#define LISTMODEDLLAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef REPLAYONLY
LISTMODEDLLAPI int LMInit (LM_stats*, CCallback*, CSinoInfo*) ;
LISTMODEDLLAPI int LMConfig (char*, unsigned long, int, int, unsigned long, Rebinner_info&) ;
LISTMODEDLLAPI int LMHalt () ;
LISTMODEDLLAPI int LMStart () ;
LISTMODEDLLAPI int LMStop () ;
LISTMODEDLLAPI int LMTriggerTimer () ;
LISTMODEDLLAPI int LMAbort () ;

#endif
LISTMODEDLLAPI int LMGetHeader (char*, CSinoInfo*) ;

#ifdef __cplusplus
}
#endif

