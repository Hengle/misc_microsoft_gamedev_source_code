p4 edit ..\..\work\shaders\tonemap\*.bin
cd tonemap
@REM /XZi /Gfp
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe"  /Gfp                                                                         /T fxl_3_0 tonemap.fx /Fo ..\..\..\work\shaders\tonemap\tonemap.bin
cd ..
