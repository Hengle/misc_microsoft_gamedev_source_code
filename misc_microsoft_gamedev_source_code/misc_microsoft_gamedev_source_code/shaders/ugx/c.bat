@REM Important: if you modify the # of registers, be sure to also change FIRST_VERTEX_SHADER_SAMPLER in ugxGeomUberSectionRenderer.cpp!
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-20,21-22 /Gfp  /DDIR_SHADOWING                                                /T fxl_3_0 parametricShader.fx /Fc parametricShader0.txt
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-20,21-22 /Gfp  /DDIR_SHADOWING  /DBUMP                                        /T fxl_3_0 parametricShader.fx /Fc parametricShader1.txt
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-20,21-22 /Gfp                                                                 /T fxl_3_0 parametricShader.fx /Fc parametricShader2.txt
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /Xsampreg:0-20,21-22 /Gfp                   /DBUMP                                        /T fxl_3_0 parametricShader.fx /Fc parametricShader3.txt




