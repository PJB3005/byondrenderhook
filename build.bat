

cl /c hooker.cpp /DEBUG:FULL /MD
link hooker.obj shlwapi.lib /SUBSYSTEM:CONSOLE /DEBUG:FULL

cl /LD hook.c D3D9.lib /MD /DEBUG:FULL