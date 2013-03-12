
DHIComps.dll: dlldata.obj DHICom_p.obj DHICom_i.obj
	link /dll /out:DHIComps.dll /def:DHIComps.def /entry:DllMain dlldata.obj DHICom_p.obj DHICom_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del DHIComps.dll
	@del DHIComps.lib
	@del DHIComps.exp
	@del dlldata.obj
	@del DHICom_p.obj
	@del DHICom_i.obj
