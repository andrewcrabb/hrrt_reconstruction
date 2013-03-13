
DSComps.dll: dlldata.obj DSCom_p.obj DSCom_i.obj
	link /dll /out:DSComps.dll /def:DSComps.def /entry:DllMain dlldata.obj DSCom_p.obj DSCom_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DSComps.dll
	@del DSComps.lib
	@del DSComps.exp
	@del dlldata.obj
	@del DSCom_p.obj
	@del DSCom_i.obj
