/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:27:36 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\Setup\DSCom.idl:
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

const IID IID_IDSMain = {0x3179ED9A,0xBDE8,0x4BBA,{0x9F,0xF6,0xD2,0x22,0xFD,0x35,0xED,0xEF}};


const IID LIBID_DSCOMLib = {0xE87B8048,0xCCDF,0x4BD5,{0x82,0x8C,0x8B,0x0F,0x16,0x9D,0x2A,0xFB}};


const CLSID CLSID_DSMain = {0xE0F5B940,0x9FDA,0x4273,{0xBA,0x79,0xB5,0x55,0x01,0x52,0x1E,0x97}};


#ifdef __cplusplus
}
#endif

