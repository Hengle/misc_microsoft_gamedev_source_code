del /F /Q *.obj
del /F /Q *.exp
del /F /Q *.dll
del /F /Q *.lib
del /F /Q *.pdb
del /F /Q *.dll
rem cl /Fd..\granny2.PDB -DGRANNY_THREADED -D_XENON -D_XBOX -D_MBCS -DNDEBUG -DGRANNY_STATISTICS -nologo -W3 -Gy -Zi -EHs-c- -O2ityp- -Ob1 -Oy -MT -c *.cpp *.c /I"C:\Program Files\Microsoft Xbox 360 SDK\include\xbox"
cl /Fd..\granny2.PDB -DGRANNY_THREADED -D_XENON -D_XBOX -D_MBCS -DNDEBUG -nologo -W3 -Gy -Zi -EHs-c- -O2ityp- -Ob1 -Oy -MT -c *.cpp *.c /I"C:\Program Files\Microsoft Xbox 360 SDK\include\xbox"
lib -nologo -subsystem:xbox *.obj *.lib -out:..\granny2.lib
copy /y granny.h ..

