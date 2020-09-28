@echo off

pushd "%~dp0"

set FTPFILE="%CD%\ftp.script"
rem build the ftp script file
echo bin>%FTPFILE%
echo cd perforce/r07.2/bin.ntx86/>>%FTPFILE%
echo get p4api_vs2003_dyn.zip>>%FTPFILE%
echo get p4api_vs2005_dyn.zip>>%FTPFILE%
echo get p4api_vs2005_dyn_vsdebug.zip>>%FTPFILE%
echo quit>>%FTPFILE%

ftp -A -s:%FTPFILE% ftp.perforce.com

rem del %FTPFILE%

if exist p4api_vs2003_dyn rd /s /q p4api_vs2003_dyn
if exist p4api_vs2005_dyn rd /s /q p4api_vs2005_dyn
if exist p4api_vs2005_dyn_vsdebug rd /s /q p4api_vs2005_dyn_vsdebug

.\unxutils\unzip p4api_vs2003_dyn.zip
move p4api-*_vs2003_dyn p4api_vs2003_dyn

.\unxutils\unzip p4api_vs2005_dyn.zip
move p4api-*_vs2005_dyn p4api_vs2005_dyn

.\unxutils\unzip p4api_vs2005_dyn_vsdebug.zip
move p4api-*_vs2005_dyn_vsdebug p4api_vs2005_dyn_vsdebug

popd