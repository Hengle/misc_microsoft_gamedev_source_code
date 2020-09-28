@echo off

rem  Create a folder called versions.
rem  Add a subfolder under each 
rem  and copy the mathcing p4/p4d into them
rem  This script will run the tests for each version.

for /d %%i in (versions\*) do (
	echo Running Version: %%~nxi
	copy /y "%%i\p4.exe" p4.exe >nul
	copy /y "%%i\p4d.exe" p4d.exe >nul
	call runtest.bat 1.1
)