/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:26:34 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\DHICom.idl:
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

const IID IID_IAbstract_DHI = {0x82C49018,0x554A,0x4AD8,{0x92,0xE9,0x25,0xE8,0x84,0x9C,0xAD,0x33}};


const IID LIBID_DHICOMLib = {0x2A00BAA8,0xC97E,0x494A,{0xA7,0x9E,0xD5,0x8C,0x2A,0x7F,0x92,0x64}};


const CLSID CLSID_Abstract_DHI = {0x96686904,0x6ECB,0x46AC,{0xB4,0xF2,0xF8,0x39,0xC6,0x1C,0x1C,0x7A}};


#ifdef __cplusplus
}
#endif

