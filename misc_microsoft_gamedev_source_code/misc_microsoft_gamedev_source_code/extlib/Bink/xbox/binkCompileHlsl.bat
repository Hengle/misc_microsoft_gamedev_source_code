p4 edit *.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T vs_2_0 /E VS_PositionAndTexCoordPassThrough bink.hlsl /Fh ..\..\extlib\Bink\xbox\VS_PositionAndTexCoordPassThrough.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PS_YCrCbToRGBNoPixelAlpha bink.hlsl /Fh ..\..\extlib\Bink\xbox\PS_YCrCbToRGBNoPixelAlpha.hlsl.h
"C:\Program Files\Microsoft Xbox 360 SDK\bin\win32\fxc.exe" /T ps_2_0 /E PS_YCrCbAToRGBA bink.hlsl /Fh ..\..\extlib\Bink\xbox\PS_YCrCbAToRGBA.hlsl.h
