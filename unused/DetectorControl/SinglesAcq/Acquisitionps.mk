
Acquisitionps.dll: dlldata.obj Acquisition_p.obj Acquisition_i.obj
	link /dll /out:Acquisitionps.dll /def:Acquisitionps.def /entry:DllMain dlldata.obj Acquisition_p.obj Acquisition_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del Acquisitionps.dll
	@del Acquisitionps.lib
	@del Acquisitionps.exp
	@del dlldata.obj
	@del Acquisition_p.obj
	@del Acquisition_i.obj
