@echo off
rem unit test script.  Usage
rem runtest.bat 1.1^|2.0 [compile]
rem
rem  The first argument is the .Net framework version
rem  Supply "compile" if you want the test harness to be compiled
rem
rem To build for the 1.1 framework, execute from a Visual Studio .Net 2003 command prompt.
rem To build for the 2.0 framework, execute from a Visual Studio .Net 2005 command prompt.
rem

set DotNetFWVersion=0
if "%~1"=="1.1" set DotNetFWVersion=1.1
if "%~1"=="2.0" set DotNetFWVersion=2.0

rem change the following line if you're really anal 
rem and want to validate the SN versions as well.
set SN=
rem set SN=SN_

if "%DotNetFWVersion%"=="0" (
	echo Must include the .Net Framework version!
	call :usage
	goto :eof
)

if "%DotNetFWVersion%"=="1.1" (
	set OutputPath=%CD%\bin\CLR_1.1
	set ReferencePath=..\bin\%SN%CLR_1.1
	set nunitPath=..\ext\nunit\1.1
)
if "%DotNetFWVersion%"=="2.0" (
	set OutputPath=%CD%\bin\CLR_2.0
	set ReferencePath=..\bin\%SN%CLR_2.0
	set nunitPath=..\ext\nunit\2.0
)

if not exist "%ReferencePath%\p4api.dll" (
	echo "Must compile p4api.dll and p4dn.dll before running test!"
	goto :eof
)

if not exist "%ReferencePath%\p4dn.dll" (
	echo "Must compile p4api.dll and p4dn.dll before running test!"
	goto :eof
)

set SrcPath=%CD%\src
set BasePath=%CD%\..

if "%~2" == "compile" call :Compile_TestHarness


call LaunchServer.bat %~3

if "%~3"=="unicode" (
    set P4CHARSET=utf8
    set P4COMMANDCHARSET=utf8
    set RUNNINGUNICODE=true
) else (
    set RUNNINGUNICODE=false
    set P4CHARSET=
    set P4COMMANDCHARSET=
)

call :RunTests

call StopServer.bat


set P4CHARSET=
set P4COMMANDCHARSET=

goto :eof


:usage
	echo.	
	echo   P4.Net Test Harness.  Usage:
	echo      runtest.bat 1.1^|2.0 [compile] [unicode]
	echo         1.1^|2.0   The .Net framework version
	echo         [compile]  Compile first (test harness only).
	echo         [unicode]  Run server in unicode mode.
	echo.
	echo      To build for the 1.1 framework, execute from a 
	echo      Visual Studio .Net 2003 command prompt.
	echo.
	echo      To build for the 2.0 framework, execute from a 
	echo      Visual Studio .Net 2005 command prompt.
	echo.
	echo      The test harness will launch a new instance of p4d 
	echo      and run tests against it.
	echo.
	echo      Your favorite version of p4d.exe and p4.exe needs 
	echo      to be added to the current directory.
	echo.
goto :eof

:Compile_TestHarness

	if exist "%OutputPath%" rd /s /q "%OutputPath%"
	mkdir "%OutputPath%"

	rem make sure we have the references
	copy /y "%ReferencePath%\p4dn.dll" "%OutputPath%\p4dn.dll" >nul
	copy /y "%ReferencePath%\p4api.dll" "%OutputPath%\p4api.dll" >nul
	copy /y "%nunitPath%\nunit.framework.dll" "%OutputPath%\nunit.framework.dll" >nul
	
	set FLAGS=/target:library 
	rem set FLAGS=%FLAGS% /reference:"%OutputPath%\p4dn.dll" 
	set FLAGS=%FLAGS% /reference:"%OutputPath%\p4api.dll" 
	set FLAGS=%FLAGS% /reference:"%OutputPath%\nunit.framework.dll" 
	set FLAGS=%FLAGS% /w:0 /o
	set FLAGS=%FLAGS% "/out:%OutputPath%\P4.Net.TestDriver.dll"
	set FLAGS=%FLAGS% /nologo 
	
	set SOURCES="%SrcPath%\P4.NetTestDriver\*.cs"
	
	echo Compiling P4.Net.TestDriver.dll...
	csc %FLAGS% %SOURCES%

goto :eof


:RunTests
	
	
	echo Running Tests...

	"%nunitPath%\nunit-console.exe" "%OutputPath%\P4.Net.TestDriver.dll" /nologo /labels /fixture=P4.Net.TestDriver.StandardTest

	if "%RUNNINGUNICODE%"=="true" (
		"%nunitPath%\nunit-console.exe" "%OutputPath%\P4.Net.TestDriver.dll" /nologo /labels /fixture=P4.Net.TestDriver.UnicodeTest
	)
goto :eof