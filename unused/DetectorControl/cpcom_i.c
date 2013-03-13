/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri May 24 15:01:59 2002
 */
/* Compiler settings for C:\Data\Backup1\Code\PVCS\P39\CC_assy\ccmon-nt\cpcom\cpcom.idl:
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

const IID IID_Icpmain = {0xFFCBB3A7,0xFF42,0x4CDE,{0xA4,0xF8,0x35,0x36,0x2D,0x33,0x57,0xD3}};


const IID IID_Idhimain = {0xE47CA878,0x87F5,0x4724,{0xA6,0xC2,0x9D,0x7D,0x10,0xBA,0xB1,0x97}};


const IID LIBID_CPCOMLib = {0xAF5A7EDA,0x96E2,0x4A84,{0xAD,0x5F,0x83,0x88,0xBB,0x98,0x80,0x7C}};


const CLSID CLSID_cpmain = {0xEC238923,0x6546,0x463D,{0x8B,0xBE,0x79,0x4D,0xDC,0x09,0x37,0xBB}};


#ifdef __cplusplus
}
#endif

