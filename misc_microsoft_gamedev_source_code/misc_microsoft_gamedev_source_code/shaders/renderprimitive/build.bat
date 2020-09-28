p4 edit ..\..\work\shaders\renderprimitive\*.bin
cd renderprimitive
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe"	/DUSE_COLOR										/Gfp /T fxl_3_0 primitive.fx /Fo ..\..\..\work\shaders\renderprimitive\primitive0.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DUSE_SCALEMATRIX /DUSE_THICKNESS_OVERRIDE      /Gfp /T fxl_3_0 primitive.fx /Fo ..\..\..\work\shaders\renderprimitive\primitive1.bin

cd ..

