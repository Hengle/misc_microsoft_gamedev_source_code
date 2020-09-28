del /F /Q *.obj
del /F /Q *.exp
del /F /Q *.dll
del /F /Q *.lib
del /F /Q *.pdb
del /F /Q *.dll
cl /Fd..\granny2D.PDB -DGRANNY_THREADED -D_XENON -D_XBOX -D_MBCS -DGRANNY_STATISTICS -nologo -W3 -Gy -Zi -EHs-c- -O2ityp- -Ob1 -Oy -MTd -c *.cpp *.c /I"C:\Program Files\Microsoft Xbox 360 SDK\include\xbox"
rem cl -DDEBUG /Fd..\granny2D.PDB -D_XENON -D_XBOX -D_MBCS -DGRANNY_STATISTICS -nologo -W3 -Gy -Zi -EHs-c- -Od -Ob1 -Oy -MTd -c *.cpp *.c /I"C:\Program Files\Microsoft Xbox 360 SDK\include\xbox"
lib -nologo -subsystem:xbox *.obj *.lib -out:..\granny2D.lib
copy /y granny.h ..

