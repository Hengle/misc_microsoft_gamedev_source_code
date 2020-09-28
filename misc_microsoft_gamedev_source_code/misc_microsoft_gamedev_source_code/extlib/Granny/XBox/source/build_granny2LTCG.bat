del /F /Q *.obj
del /F /Q *.exp
del /F /Q *.dll
del /F /Q *.lib
del /F /Q *.pdb
del /F /Q *.dll

@echo off
rem ***  Do not use the /Ot (to favor fast code) option since it causes really bad things (SAT) ***

@echo on
cl /GL /Fd..\granny2LTCG.PDB -DGRANNY_THREADED -D_XENON -D_XBOX -D_MBCS -DNDEBUG -nologo -W3 -Gy -Zi -EHs-c- -Ox -Ob2 -Oi -Os -MT -c *.cpp *.c /I"C:\Program Files\Microsoft Xbox 360 SDK\include\xbox"
lib -nologo -subsystem:xbox *.obj *.lib -out:..\granny2LTCG.lib
copy /y granny.h ..

