cl /c hooker.cpp /Os /MD
link hooker.obj shlwapi.lib /SUBSYSTEM:CONSOLE

cl /c hook.c /Os /MD
link /DLL hook.obj D3D9.lib