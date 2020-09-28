p4 edit ..\..\work\shaders\terrain\*.bin
cd terrain
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "gpuTerrainComposite.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fo ..\..\..\work\shaders\terrain\gpuTerrainComposite.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" "gpuTerrainComposite.fx" /Gfp /Xsampreg:0-15,16-24 /T fxl_3_0 /Fc gpuTerrainComposite.asm 
cd ..
