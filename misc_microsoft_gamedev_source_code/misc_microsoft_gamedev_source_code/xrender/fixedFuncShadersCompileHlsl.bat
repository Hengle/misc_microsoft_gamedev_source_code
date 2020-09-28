p4 edit *.hlsl.h

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VSFixedFunc fixedFuncShaders.hlsl /Fh fixedFuncVSPos.hlsl.h /Vn g_xvs_PosVS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VSFixedFunc /D UV0 fixedFuncShaders.hlsl /Fh fixedFuncVSPosTex1.hlsl.h /Vn g_xvs_PosTex1VS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VSFixedFunc /D DIFFUSE fixedFuncShaders.hlsl /Fh fixedFuncVSPosDiffuse.hlsl.h /Vn g_xvs_PosDiffuseVS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VSFixedFunc /D DIFFUSE /D UV0 fixedFuncShaders.hlsl /Fh fixedFuncVSPosDiffuseTex1.hlsl.h /Vn g_xvs_PosDiffuseTex1VS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VSFixedFunc /D DIFFUSE /D UV0 /D UV1 fixedFuncShaders.hlsl /Fh fixedFuncVSPosDiffuseTex2.hlsl.h /Vn g_xvs_PosDiffuseTex2VS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VSFixedFunc /D USE_CONSTANT_DIFFUSE fixedFuncShaders.hlsl /Fh fixedFuncVSPosConstantDiffuse.hlsl.h /Vn g_xvs_PosConstantDiffuseVS

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc /D DIFFUSE fixedFuncShaders.hlsl /Fh fixedFuncPSDiffuse.hlsl.h /Vn g_xps_DiffusePS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc /D DIFFUSE /D UV0 fixedFuncShaders.hlsl /Fh fixedFuncPSDiffuseTex1.hlsl.h /Vn g_xps_DiffuseTex1PS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc /D DIFFUSE /D UV0 /D UV1 fixedFuncShaders.hlsl /Fh fixedFuncPSDiffuseTex2.hlsl.h /Vn g_xps_DiffuseTex2PS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc /D DIFFUSE /D UV0 /D OVERBRIGHT fixedFuncShaders.hlsl /Fh fixedFuncPSDiffuseTex1OverBright.hlsl.h /Vn g_xps_DiffuseTex1PSOverBright

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc fixedFuncShaders.hlsl /Fh fixedFuncPSWhite.hlsl.h /Vn g_xps_WhitePS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc /D UV0 fixedFuncShaders.hlsl /Fh fixedFuncPSTex1.hlsl.h /Vn g_xps_Tex1PS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSFixedFunc /D UV0 /D UV1 fixedFuncShaders.hlsl /Fh fixedFuncPSTex2.hlsl.h /Vn g_xps_Tex2PS

"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSDepthVis fixedFuncShaders.hlsl /Fh fixedFuncPSDepthVis.hlsl.h /Vn g_xps_DepthVisPS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSAlphaVis fixedFuncShaders.hlsl /Fh fixedFuncPSAlphaVis.hlsl.h /Vn g_xps_AlphaVisPS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSRedVis fixedFuncShaders.hlsl /Fh fixedFuncPSRedVis.hlsl.h /Vn g_xps_RedVisPS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSGreenVis fixedFuncShaders.hlsl /Fh fixedFuncPSGreenVis.hlsl.h /Vn g_xps_GreenVisPS
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PSBlueVis fixedFuncShaders.hlsl /Fh fixedFuncPSBlueVis.hlsl.h /Vn g_xps_BlueVisPS
