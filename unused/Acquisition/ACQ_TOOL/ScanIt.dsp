# Microsoft Developer Studio Project File - Name="ScanIt" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ScanIt - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ScanIt.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ScanIt.mak" CFG="ScanIt - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ScanIt - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ScanIt - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ScanIt - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I "..\include" /I "..\..\gantryModel\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "NEWACS_SUPPORT" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 gm328.lib listmode_u.lib histogram.lib gantryModel.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /out:"Release/ScanIt_u.exe" /libpath:"..\lib"

!ELSEIF  "$(CFG)" == "ScanIt - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\include" /I "..\..\gantryModel\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 listmode.lib histogram.lib gantryModel.lib ws2_32.lib /nologo /subsystem:windows /profile /debug /machine:I386 /libpath:"..\lib"

!ENDIF 

# Begin Target

# Name "ScanIt - Win32 Release"
# Name "ScanIt - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CDHI.cpp
# End Source File
# Begin Source File

SOURCE=.\CommSetup.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Diagnostic.cpp
# End Source File
# Begin Source File

SOURCE=.\DosageInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\GraphProp.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Param.cpp
# End Source File
# Begin Source File

SOURCE=.\Patient.cpp
# End Source File
# Begin Source File

SOURCE=.\ScanIt.cpp
# End Source File
# Begin Source File

SOURCE=.\ScanIt.rc
# End Source File
# Begin Source File

SOURCE=.\ScanItDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ScanItView.cpp
# End Source File
# Begin Source File

SOURCE=.\SerialBus.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CDHI.h
# End Source File
# Begin Source File

SOURCE=.\CommSetup.h
# End Source File
# Begin Source File

SOURCE=.\ConfigDlg.h
# End Source File
# Begin Source File

SOURCE=.\Diagnostic.h
# End Source File
# Begin Source File

SOURCE=.\DosageInfo.h
# End Source File
# Begin Source File

SOURCE=.\ECodes.h
# End Source File
# Begin Source File

SOURCE=.\GraphProp.h
# End Source File
# Begin Source File

SOURCE=.\ISO_Data.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Param.h
# End Source File
# Begin Source File

SOURCE=.\Patient.h
# End Source File
# Begin Source File

SOURCE=.\PatientADO.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ScanIt.h
# End Source File
# Begin Source File

SOURCE=.\ScanItDoc.h
# End Source File
# Begin Source File

SOURCE=.\ScanItView.h
# End Source File
# Begin Source File

SOURCE=.\SerialBus.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\CONTROL.ICO
# End Source File
# Begin Source File

SOURCE=.\res\CTI.ico
# End Source File
# Begin Source File

SOURCE=.\res\DISK04.ICO
# End Source File
# Begin Source File

SOURCE=.\res\GRAPHPRO.ICO
# End Source File
# Begin Source File

SOURCE=.\res\Icon1.ico
# End Source File
# Begin Source File

SOURCE=.\res\icon16.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icon2.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icon3.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icon4.ico
# End Source File
# Begin Source File

SOURCE=.\res\Icon5.ico
# End Source File
# Begin Source File

SOURCE=.\res\MISC01.ICO
# End Source File
# Begin Source File

SOURCE=.\res\MISC04.ICO
# End Source File
# Begin Source File

SOURCE=.\res\MISC39B.ICO
# End Source File
# Begin Source File

SOURCE=.\res\MSGBOX01.ICO
# End Source File
# Begin Source File

SOURCE=.\res\OPENFOLD.ICO
# End Source File
# Begin Source File

SOURCE=.\res\PHONE1.ICO
# End Source File
# Begin Source File

SOURCE=.\res\ScanIt.ico
# End Source File
# Begin Source File

SOURCE=.\res\ScanIt.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ScanItDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\TOOLS.ICO
# End Source File
# Begin Source File

SOURCE=.\res\TRANSFRM.ICO
# End Source File
# Begin Source File

SOURCE=.\res\WIZARD.ICO
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
