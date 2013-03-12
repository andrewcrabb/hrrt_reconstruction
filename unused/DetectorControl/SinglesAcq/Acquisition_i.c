/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:27:07 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\SinglesAcq\Acquisition.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_IACQMain = {0x28874B6F,0x9FFA,0x4898,{0x95,0xEA,0x82,0x3F,0x1E,0xE1,0x1A,0x3F}};


const IID LIBID_ACQUISITIONLib = {0x98E8B06F,0x561D,0x48DA,{0x9A,0x94,0x6A,0x32,0x1F,0x3B,0x53,0xD8}};


const CLSID CLSID_ACQMain = {0xE8A509A4,0x3C3F,0x4348,{0x8E,0xF3,0x08,0x49,0x92,0xDD,0xDC,0x78}};


#ifdef __cplusplus
}
#endif

