del /F /Q *.obj
del /F /Q *.exp
del /F /Q *.dll
del /F /Q *.lib
del /F /Q *.pdb
del /F /Q *.dll
rem cl -DSTRICT -D_WIN32 -DWIN32 -D_WINDOWS -DWIN32_LEAN_AND_MEAN -DNDEBUG -DGRANNY_STATISTICS -nologo -W4 -Gy -Gi- -GX- -Zi -O2gityp- -G6s -QIfdiv- -Ob1 -Oy -c *.cpp *.c
rem link -LIBPATH:C:\dev\rad\bin\devstu6\vc98\lib -link50compat -nologo -incremental:no -map -debug:full -OPT:ref -OPT:nowin98 -BASE:0x50000000 -NODEFAULTLIB -subsystem:windows -dll -out:granny2.dll kernel32.lib user32.lib gdi32.lib advapi32.lib winmm.lib wsock32.lib glu32.lib shell32.lib version.lib comctl32.lib comdlg32.lib opengl32.lib *.obj *.lib

cl -DSTRICT -D_WIN32 -DWIN32 -D_WINDOWS -DWIN32_LEAN_AND_MEAN -DNDEBUG -DRAD_NO_LOWERCASE_TYPES -DGRANNY_STATISTICS -nologo -W4 -wd4514 -Wp64 -Gy -Gi- -GX- -Zi -O2gityp- -G6s -QIfdiv- -Ob1 -Oy -c *.cpp *.c
link -LIBPATH:C:\Progra~1\MICROS~1.NET\vc7\Platfo~1\lib -LIBPATH:C:\Progra~1\MICROS~1.NET\vc7\lib -nologo -incremental:no -map -debug:full -OPT:ref -OPT:nowin98 -BASE:0x50000000 -NODEFAULTLIB -subsystem:windows -dll -out:granny2.dll kernel32.lib user32.lib ole32.lib gdi32.lib advapi32.lib winmm.lib wsock32.lib glu32.lib shell32.lib version.lib comctl32.lib comdlg32.lib opengl32.lib *.obj *.lib

copy /y *.lib ..
copy /y *.dll ..
copy /y *.pdb ..
copy /y granny.h ..
