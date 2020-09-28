@echo off
p4 edit ..\..\work\shaders\dxtPack\*.bin
cd dxtPack

rem "C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "dxtqUnpack.fx" /XZi /XOd /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fc dxtqUnpack.asm 
rem "C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "dxtqUnpack.fx" /XZi /XOd /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fo ..\..\..\work\shaders\dxtPack\dxtqUnpack.bin

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "dxtqUnpack.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fc dxtqUnpack.asm 
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "dxtqUnpack.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fo ..\..\..\work\shaders\dxtPack\dxtqUnpack.bin

cd ..
