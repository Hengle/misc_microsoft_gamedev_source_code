@echo off
p4 edit ..\..\work\shaders\dxtPack\*.bin
cd dxtPack

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "dxtPack.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fc dxtPack.asm 
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "dxtPack.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fo ..\..\..\work\shaders\dxtPack\dxtPack.bin

rem "..\..\tools\fxc4025\fxc4025.exe" "dxtPack.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fc dxtPack.asm 
rem "..\..\tools\fxc4025\fxc4025.exe" "dxtPack.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fo ..\..\..\work\shaders\dxtPack\dxtPack.bin

cd ..
