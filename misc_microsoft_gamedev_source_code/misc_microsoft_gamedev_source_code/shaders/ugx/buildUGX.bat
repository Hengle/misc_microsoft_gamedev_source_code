@REM Important: if you modify the # of registers, be sure to also change FIRST_VERTEX_SHADER_SAMPLER in ugxGeomUberSectionRenderer.cpp!
p4 edit ..\..\work\shaders\ugx\*.bin
cd ugx
@REM /XZi /Gfp
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-20,21-22 /DDIR_SHADOWING                                                        /T fxl_3_0 parametricShader.fx /Fo ..\..\..\work\shaders\ugx\parametricShader0.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-20,21-22 /DDIR_SHADOWING          /DBUMP                                        /T fxl_3_0 parametricShader.fx /Fo ..\..\..\work\shaders\ugx\parametricShader1.bin
rem "C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-18,18-19                                                                        /T fxl_3_0 parametricShader.fx /Fo ..\..\..\work\shaders\ugx\parametricShader2.bin
rem "C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-18,18-19                          /DBUMP                                        /T fxl_3_0 parametricShader.fx /Fo ..\..\..\work\shaders\ugx\parametricShader3.bin
cd ..
