# Microsoft Developer Studio Project File - Name="DUGUI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=DUGUI - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DUGUI.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DUGUI.mak" CFG="DUGUI - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DUGUI - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "DUGUI - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "DUGUI - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "INCLUDE_DUCOM_I" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 gantryModel.lib ErrorEventSupport.lib /nologo /stack:0xa00000 /subsystem:windows /debug /debugtype:coff /machine:I386

!ELSEIF  "$(CFG)" == "DUGUI - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 gantryModel.lib /nologo /stack:0xa00000 /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "DUGUI - Win32 Release"
# Name "DUGUI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\CRMEdit.cpp
# End Source File
# Begin Source File

SOURCE=.\CrystalPeaks.cpp
# End Source File
# Begin Source File

SOURCE=.\CrystalTime.cpp
# End Source File
# Begin Source File

SOURCE=.\DesignatePeaks.cpp
# End Source File
# Begin Source File

SOURCE=.\DUGUI.cpp
# End Source File
# Begin Source File

SOURCE=.\DUGUI.rc
# End Source File
# Begin Source File

SOURCE=.\DUGUIDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EnergyRange.cpp
# End Source File
# Begin Source File

SOURCE=..\Setup\find_peak_1d.cpp
# End Source File
# Begin Source File

SOURCE=.\HeadSelection.cpp
# End Source File
# Begin Source File

SOURCE=.\InfoDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\LoadDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\Setup\position_profile_tools.cpp
# End Source File
# Begin Source File

SOURCE=.\remove_dnl.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Validate.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\CRMEdit.h
# End Source File
# Begin Source File

SOURCE=.\CrystalPeaks.h
# End Source File
# Begin Source File

SOURCE=.\CrystalTime.h
# End Source File
# Begin Source File

SOURCE=.\DesignatePeaks.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Include\DHI_Constants.h
# End Source File
# Begin Source File

SOURCE=.\DUGUI.h
# End Source File
# Begin Source File

SOURCE=.\DUGUIDlg.h
# End Source File
# Begin Source File

SOURCE=.\EnergyRange.h
# End Source File
# Begin Source File

SOURCE=.\find_peak_1d.h
# End Source File
# Begin Source File

SOURCE=.\HeadSelection.h
# End Source File
# Begin Source File

SOURCE=.\InfoDialog.h
# End Source File
# Begin Source File

SOURCE=.\position_profile_tools.h
# End Source File
# Begin Source File

SOURCE=.\remove_dnl.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\Include\Setup_Constants.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Validate.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\DUGUI.ico
# End Source File
# Begin Source File

SOURCE=.\res\DUGUI.rc2
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
