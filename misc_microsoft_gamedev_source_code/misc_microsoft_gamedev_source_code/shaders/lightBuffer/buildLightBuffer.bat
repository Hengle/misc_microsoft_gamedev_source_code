p4 edit ..\..\work\shaders\lightBuffer\*.bin
cd lightBuffer
rem "C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe"  /XZi /T fxl_3_0 lightBuffer.fx /Fo ..\..\..\work\shaders\lightBuffer\lightBuffer.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe"   /T fxl_3_0 lightBuffer.fx /Fo ..\..\..\work\shaders\lightBuffer\lightBuffer.bin

cd ..
