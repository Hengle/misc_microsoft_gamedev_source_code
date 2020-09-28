p4 edit *.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E AtgFontVertexShader Atg.hlsl /Fh AtgFontVertexShader.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E AtgFontPixelShader Atg.hlsl /Fh AtgFontPixelShader.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E AtgGradientVertexShader Atg.hlsl /Fh AtgGradientVertexShader.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E AtgGradientPixelShader Atg.hlsl /Fh AtgGradientPixelShader.hlsl.h
