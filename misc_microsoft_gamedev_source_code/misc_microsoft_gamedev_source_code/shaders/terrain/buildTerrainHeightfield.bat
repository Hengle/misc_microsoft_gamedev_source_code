p4 edit ..\..\work\shaders\terrain\*.bin
cd terrain

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "terrainHeightField.fx" /XZi /Xsampreg:0-15,16-25 /T fxl_3_0 /Fo ..\..\..\work\shaders\terrain\terrainHeightField.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "terrainHeightField.fx" /XZi /Xsampreg:0-15,16-25 /T fxl_3_0 /Fc terrainHeightField.asm 

cd ..
