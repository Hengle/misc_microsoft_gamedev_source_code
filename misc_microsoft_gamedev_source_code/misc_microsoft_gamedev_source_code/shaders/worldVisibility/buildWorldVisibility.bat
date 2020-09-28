p4 edit ..\..\work\shaders\worldVisibility\*.bin
cd worldVisibility
rem "C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe"  /XZi /XOd /T fxl_3_0 worldVisibility.fx /Fo ..\..\..\work\shaders\worldVisibility\worldVisibility.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe"  /T fxl_3_0 worldVisibility.fx /Fo ..\..\..\work\shaders\worldVisibility\worldVisibility.bin

cd ..
