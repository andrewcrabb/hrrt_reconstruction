; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CPlotOpt
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "CorrectRun.h"
LastPage=0

ClassCount=7
Class1=CCorrectRunApp
Class2=CCorrectRunDoc
Class3=CCorrectRunView
Class4=CMainFrame

ResourceCount=5
Resource1=IDD_USERIN_DIALOG
Resource2=IDD_CORRECTRUN_FORM
Class5=CAboutDlg
Resource3=IDD_ABOUTBOX
Class6=CUserIn
Resource4=IDR_MAINFRAME
Class7=CPlotOpt
Resource5=IDD_GRAPH_OPTION

[CLS:CCorrectRunApp]
Type=0
HeaderFile=CorrectRun.h
ImplementationFile=CorrectRun.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC
LastObject=CCorrectRunApp

[CLS:CCorrectRunDoc]
Type=0
HeaderFile=CorrectRunDoc.h
ImplementationFile=CorrectRunDoc.cpp
Filter=N
BaseClass=CDocument
VirtualFilter=DC
LastObject=CCorrectRunDoc

[CLS:CCorrectRunView]
Type=0
HeaderFile=CorrectRunView.h
ImplementationFile=CorrectRunView.cpp
Filter=D
BaseClass=CFormView
VirtualFilter=VWC
LastObject=IDC_EDIT3


[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
LastObject=CMainFrame
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=CorrectRun.cpp
ImplementationFile=CorrectRun.cpp
Filter=D
LastObject=CAboutDlg

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_OPEN
Command2=ID_APP_EXIT
Command3=IDD_USER_DIALOG
Command4=ID_CONTROL_PLOTOPT
Command5=ID_APP_ABOUT
CommandCount=5

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_FILE_OPEN
Command3=ID_FILE_SAVE
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_CUT
Command6=ID_EDIT_COPY
Command7=ID_EDIT_PASTE
Command8=ID_EDIT_UNDO
Command9=ID_EDIT_CUT
Command10=ID_EDIT_COPY
Command11=ID_EDIT_PASTE
Command12=ID_NEXT_PANE
Command13=ID_PREV_PANE
CommandCount=13

[DLG:IDD_CORRECTRUN_FORM]
Type=1
Class=CCorrectRunView
ControlCount=9
Control1=IDC_EDIT1,edit,1350633600
Control2=IDC_EDIT3,edit,1350633600
Control3=IDC_EDIT_ACTIVITY,edit,1350633600
Control4=IDC_STATIC,static,1342308865
Control5=IDC_EDIT5,edit,1350633600
Control6=IDC_EDIT_STRENGTH,edit,1350633600
Control7=IDC_EDIT_PERCENT,edit,1350633600
Control8=IDC_EDIT_RADUIS,edit,1350633600
Control9=IDC_PROGRESS1,msctls_progress32,1073741825

[DLG:IDD_USERIN_DIALOG]
Type=1
Class=CUserIn
ControlCount=20
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_STATIC,static,1350701068
Control4=IDC_STATIC,static,1350701068
Control5=IDC_STATIC,static,1350701068
Control6=IDC_COMBO1,combobox,1344340226
Control7=IDC_EDIT1,edit,1350631552
Control8=IDC_COMBO2,combobox,1344340226
Control9=IDC_EDIT3,edit,1350631552
Control10=IDC_COMBO3,combobox,1344340226
Control11=IDC_CALENDAR1,{8E27C92B-1264-101C-8A2F-040224009C02},1342242816
Control12=IDC_STATIC,static,1342308865
Control13=IDC_STATIC,static,1342308865
Control14=IDC_STATIC,static,1342308865
Control15=IDC_STATIC,static,1342308865
Control16=IDC_HOUR_COMBO,combobox,1344339970
Control17=IDC_EDIT_MINUTE,edit,1350631552
Control18=IDC_EDIT_SECOND,edit,1350631552
Control19=IDC_STATIC,static,1350701056
Control20=IDC_EDIT7,edit,1350631552

[CLS:CUserIn]
Type=0
HeaderFile=UserIn.h
ImplementationFile=UserIn.cpp
BaseClass=CDialog
Filter=D
LastObject=CUserIn
VirtualFilter=dWC

[DLG:IDD_GRAPH_OPTION]
Type=1
Class=CPlotOpt
ControlCount=21
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_CHECK1,button,1342242819
Control4=IDC_CHECK2,button,1342242819
Control5=IDC_STATIC,button,1342177287
Control6=IDC_STATIC,button,1342242823
Control7=IDC_RADIO_SCALE,button,1342373897
Control8=IDC_RADIO_SCALE2,button,1342242825
Control9=IDC_EDIT1,edit,1350631552
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_EDIT2,edit,1350631552
Control13=IDC_STATIC,button,1342242823
Control14=IDC_GRAPH_TYPE,button,1342373897
Control15=IDC_GRAPH_TYPE2,button,1342242825
Control16=IDC_LIN_LOG,button,1073938441
Control17=IDC_LIN_LOG2,button,1073807369
Control18=IDC_STATIC,static,1342308352
Control19=IDC_EDIT5,edit,1350631552
Control20=IDC_STATIC,static,1342308352
Control21=IDC_EDIT8,edit,1350631552

[CLS:CPlotOpt]
Type=0
HeaderFile=PlotOpt.h
ImplementationFile=PlotOpt.cpp
BaseClass=CDialog
Filter=D
LastObject=CPlotOpt
VirtualFilter=dWC

