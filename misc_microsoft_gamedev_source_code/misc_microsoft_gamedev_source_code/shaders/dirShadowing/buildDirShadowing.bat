p4 edit ..\..\work\shaders\dirShadowing\*.bin
cd dirShadowing
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T fxl_3_0 dirShadowing.fx /Fo ..\..\..\work\shaders\dirShadowing\dirShadowing.bin
cd ..
