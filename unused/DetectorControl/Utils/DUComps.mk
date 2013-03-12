
DUComps.dll: dlldata.obj DUCom_p.obj DUCom_i.obj
	link /dll /out:DUComps.dll /def:DUComps.def /entry:DllMain dlldata.obj DUCom_p.obj DUCom_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DUComps.dll
	@del DUComps.lib
	@del DUComps.exp
	@del dlldata.obj
	@del DUCom_p.obj
	@del DUCom_i.obj
