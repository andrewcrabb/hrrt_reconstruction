/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:27:19 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\Utils\DUCom.idl:
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

const IID IID_IDUMain = {0xC0AA4C6E,0x2E3E,0x4512,{0x84,0x35,0x11,0x4C,0xE2,0x7E,0x07,0xFD}};


const IID LIBID_DUCOMLib = {0x5224341A,0xA9F7,0x4094,{0x88,0x51,0xFA,0xE7,0x6C,0x10,0x41,0x04}};


const CLSID CLSID_DUMain = {0x3FA04976,0x1CFD,0x4C2E,{0xA4,0xE7,0x86,0x5D,0x20,0x8D,0xCC,0x3E}};


#ifdef __cplusplus
}
#endif

