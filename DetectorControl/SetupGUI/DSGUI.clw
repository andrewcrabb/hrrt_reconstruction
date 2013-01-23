; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CDSGUIDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "DSGUI.h"

ClassCount=4
Class1=CDSGUIApp
Class2=CDSGUIDlg
Class3=CAboutDlg

ResourceCount=4
Resource1=IDD_ABOUTBOX
Resource2=IDR_MAINFRAME
Resource3=IDD_DSGUI_DIALOG
Class4=CLoadDlg
Resource4=IDD_LOAD_DIALOG

[CLS:CDSGUIApp]
Type=0
HeaderFile=DSGUI.h
ImplementationFile=DSGUI.cpp
Filter=N

[CLS:CDSGUIDlg]
Type=0
HeaderFile=DSGUIDlg.h
ImplementationFile=DSGUIDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDCONTINUE

[CLS:CAboutDlg]
Type=0
HeaderFile=DSGUIDlg.h
ImplementationFile=DSGUIDlg.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_DSGUI_DIALOG]
Type=1
Class=CDSGUIDlg
ControlCount=25
Control1=IDEXIT,button,1342242816
Control2=IDC_HEADGRID,{6262D3A0-531B-11CF-91F6-C2863C385E30},1342242816
Control3=IDC_HEADLIST,listbox,1350631691
Control4=IDC_DROPHEAD,combobox,1344340226
Control5=IDC_DROPTYPE,combobox,1344339970
Control6=IDC_STATIC,static,1342308352
Control7=IDC_DROP_CONFIG,combobox,1344339970
Control8=IDC_STATIC,static,1342308352
Control9=IDC_DROP_EMTX,combobox,1344339970
Control10=IDC_STATIC,static,1342308352
Control11=IDRESET,button,1342242816
Control12=IDABORT,button,1342242816
Control13=IDC_TOTAL_PROGRESS,msctls_progress32,1350565888
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STAGE_PROGRESS,msctls_progress32,1350565888
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STAGE_LABEL,static,1342308352
Control18=IDSTART,button,1342242816
Control19=IDARCHIVE,button,1342242816
Control20=IDSET,button,1342242816
Control21=IDCLEAR,button,1342242816
Control22=IDC_STATUS1,static,1342308352
Control23=IDC_STATUS2,static,1342308352
Control24=IDC_STATUS3,static,1342308352
Control25=IDCONTINUE,button,1342242816

[DLG:IDD_LOAD_DIALOG]
Type=1
Class=CLoadDlg
ControlCount=11
Control1=IDLOAD,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_HEAD_LIST,listbox,1352728843
Control4=IDC_ARCHIVE_LIST,listbox,1352728835
Control5=IDC_MESSAGE_LABEL,static,1342308352
Control6=IDC_LOAD_CONFIG,combobox,1344340226
Control7=IDC_STATIC,static,1342308352
Control8=IDSAVE,button,1342242817
Control9=IDDELETE,button,1342242817
Control10=IDC_STATIC,static,1342308352
Control11=IDABORT,button,1342242817

[CLS:CLoadDlg]
Type=0
HeaderFile=LoadDlg.h
ImplementationFile=LoadDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=IDC_MESSAGE_LABEL
VirtualFilter=dWC

