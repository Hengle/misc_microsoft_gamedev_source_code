p4 edit ..\..\work\shaders\particles\*.bin
cd particles
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBILLBOARD			               														/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle0.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DUPFACING		                  														/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle1.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAIL /DTRAIL_COMPUTE_FORWARD /DFETCH_OCTSTRIP	      								/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle2.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DAXIAL	                        														/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle3.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAIL /DTRAIL_COMPUTE_FORWARD /DFETCH_OCTSTRIP /DTRAIL_STRETCHED_TEXTURE 			/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle4.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS1 /DTRAIL_COMPUTE_FORWARD /DFETCH_OCTSTRIP 							/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle5.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS2 /DTRAIL_COMPUTE_FORWARD /DFETCH_OCTSTRIP 							/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle6.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS1 /DTRAIL_COMPUTE_FORWARD /DFETCH_OCTSTRIP /DTRAIL_STRETCHED_TEXTURE	/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle7.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS2 /DTRAIL_COMPUTE_FORWARD /DFETCH_OCTSTRIP /DTRAIL_STRETCHED_TEXTURE	/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle8.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBEAM /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE /Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle9.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DVELOCITYALIGNED                  														/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle10.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTERRAIN_PATCH                  														/Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle11.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBEAM /DBEAM_VERTICAL /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE /Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle12.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBEAM /DBEAM_HORIZONTAL /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE /Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle13.bin

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBILLBOARD			               														/Gfp /T fxl_3_0 particle.fx /Fc particle0.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DUPFACING		                  														/Gfp /T fxl_3_0 particle.fx /Fc particle1.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAIL /DTRAIL_COMPUTE_FORWARD /DFETCH_QUADSTRIP	      								/Gfp /T fxl_3_0 particle.fx /Fc particle2.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DAXIAL	                        														/Gfp /T fxl_3_0 particle.fx /Fc particle3.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAIL /DTRAIL_COMPUTE_FORWARD /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE 			/Gfp /T fxl_3_0 particle.fx /Fc particle4.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS1 /DTRAIL_COMPUTE_FORWARD /DFETCH_QUADSTRIP 							/Gfp /T fxl_3_0 particle.fx /Fc particle5.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS2 /DTRAIL_COMPUTE_FORWARD /DFETCH_QUADSTRIP 							/Gfp /T fxl_3_0 particle.fx /Fc particle6.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS1 /DTRAIL_COMPUTE_FORWARD /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE	/Gfp /T fxl_3_0 particle.fx /Fc particle7.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTRAILCROSS_PASS2 /DTRAIL_COMPUTE_FORWARD /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE	/Gfp /T fxl_3_0 particle.fx /Fc particle8.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBEAM /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE /Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle9.bin
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DVELOCITYALIGNED		                  														/Gfp /T fxl_3_0 particle.fx /Fc particle10.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DTERRAIN_PATCH		                  														/Gfp /T fxl_3_0 particle.fx /Fc particle11.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBEAM /DBEAM_VERTICAL /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE /Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle12.asm
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /DBEAM /DBEAM_HORIZONTAL /DFETCH_QUADSTRIP /DTRAIL_STRETCHED_TEXTURE /Gfp /T fxl_3_0 particle.fx /Fo ..\..\..\work\shaders\particles\particle13.asm

cd ..

