:: %1 - Whether or not to build LTCG

:: ------------------------------------------------------------
:: Playtest build
:: ------------------------------------------------------------
if exist ..\..\..\code\xgame\xbox\playtest\xgameP.xex del /F ..\..\..\code\xgame\xbox\playtest\xgameP.xex
if exist ..\..\..\code\xgame\xbox\playtest\xgameP.exe del /F ..\..\..\code\xgame\xbox\playtest\xgameP.exe
buildconsole ..\..\..\code\xgame\xgame.sln /CFG="Playtest|Xbox 360" /LOG=build_playtest.log /CL_ADD="/DUSE_BUILD_INFO" /REBUILD
if not exist ..\..\..\code\xgame\xbox\playtest\xgameP.xex goto error_playtest_build

:: ------------------------------------------------------------
:: Debug build
:: ------------------------------------------------------------
if exist ..\..\..\code\xgame\xbox\debug\xgameD.xex del /F ..\..\..\code\xgame\xbox\debug\xgameD.xex
if exist ..\..\..\code\xgame\xbox\debug\xgameD.exe del /F ..\..\..\code\xgame\xbox\debug\xgameD.exe
buildconsole ..\..\..\code\xgame\xgame.sln /CFG="Debug|Xbox 360" /LOG=build_debug.log /CL_ADD="/DUSE_BUILD_INFO" /REBUILD
if not exist ..\..\..\code\xgame\xbox\debug\xgameD.xex goto error_debug_build

:: ------------------------------------------------------------
:: Final build
:: ------------------------------------------------------------
if exist ..\..\..\code\xgame\xbox\final\xgameF.xex del /F ..\..\..\code\xgame\xbox\final\xgameF.xex
if exist ..\..\..\code\xgame\xbox\final\xgameF.exe del /F ..\..\..\code\xgame\xbox\final\xgameF.exe
buildconsole ..\..\..\code\xgame\xgame.sln /CFG="Final|Xbox 360" /LOG=build_final.log /CL_ADD="/DUSE_BUILD_INFO" /REBUILD
if not exist ..\..\..\code\xgame\xbox\final\xgameF.xex goto error_final_build

:: ------------------------------------------------------------
:: Profile build
:: ------------------------------------------------------------
if exist ..\..\..\code\xgame\xbox\profile\xgameProfile.xex del /F ..\..\..\code\xgame\xbox\profile\xgameProfile.xex
if exist ..\..\..\code\xgame\xbox\profile\xgameProfile.exe del /F ..\..\..\code\xgame\xbox\profile\xgameProfile.exe
buildconsole ..\..\..\code\xgame\xgame.sln /CFG="Profile|Xbox 360" /LOG=build_profile.log /CL_ADD="/DUSE_BUILD_INFO" /REBUILD
if not exist ..\..\..\code\xgame\xbox\profile\xgameProfile.xex goto error_profile_build



:: ------------------------------------------------------------
:: LTCG Build
:: ------------------------------------------------------------

if %1=0 goto skipLTCGBuild

set PATH=%XEDK%\bin\win32;%PATH%;"C:\Program Files (x86)\Microsoft Visual Studio 8\Common7\IDE"
set INCLUDE=%XEDK%\include\win32;%XEDK%\include\xbox;%XEDK%\include\xbox\sys;%INCLUDE%
set LIB=%XEDK%\lib\win32;%XEDK%\lib\xbox;%LIB%
set _NT_DEBUG_1394_CHANNEL=1
set _NT_DEBUG_BUS=1394
set _NT_SYMBOL_PATH=SRV*%XEDK%\bin\xbox\symsrv;%_NT_SYMBOL_PATH%

if exist ..\..\..\code\xgame\xbox\final_LTCG\xgameFLTCG.xex del /F ..\..\..\code\xgame\xbox\final_LTCG\xgameFLTCG.xex
if exist ..\..\..\code\xgame\xbox\final_LTCG\xgameFLTCG.exe del /F ..\..\..\code\xgame\xbox\final_LTCG\xgameFLTCG.exe
devenv.com ..\..\..\code\xgame\xgame.sln /Rebuild Final_LTCG /Out build_ltcg.log
if not exist ..\..\..\code\xgame\xbox\final_LTCG\xgameFLTCG.xex goto error_ltcg_build

:skipLTCGBuild

:done

