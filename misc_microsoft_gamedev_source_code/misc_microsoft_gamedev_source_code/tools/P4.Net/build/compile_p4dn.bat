@echo off
rem Primary build script.  Usage
rem build.bat 1.1^|2.0 [SN]
rem
rem  The first argument is the .Net framework version
rem  Supply SN as the second argument to strong-name the assemblies.
rem
rem To build for the 1.1 framework, execute from a Visual Studio .Net 2003 command prompt.
rem To build for the 2.0 framework, execute from a Visual Studio .Net 2005 command prompt.
rem

set DotNetFWVersion=0
if "%~1"=="1.1" set DotNetFWVersion=1.1
if "%~1"=="2.0" set DotNetFWVersion=2.0
if "%~2"=="SN" (
	set CompilingSN=true
) else (
	set CompilingSN=false
)

if "%DotNetFWVersion%"=="0" (
	echo Must include the .Net Framework version!
	call :usage
	goto :eof
)

if "%DotNetFWVersion%"=="1.1" (
	if "%~2"=="SN" (
		set OutputPath=%CD%\..\bin\SN_CLR_1.1
	) else (
		set OutputPath=%CD%\..\bin\CLR_1.1
	)
)
if "%DotNetFWVersion%"=="2.0" (
	if "%~2"=="SN" (
		set OutputPath=%CD%\..\bin\SN_CLR_2.0
	) else (
		set OutputPath=%CD%\..\bin\CLR_2.0
	)
)

set SrcPath=%CD%\..\src
set BasePath=%CD%\..


call :Compile_P4DN_Assembly
call :Compile_P4API_Assembly

if exist "%OutputPath%\P4.Net.snk" del "%OutputPath%\P4.Net.snk"

goto :eof


:usage
	echo build.bat 1.1^|2.0 [SN]
	echo.
	echo   The first argument is the .Net framework version
	echo   The second argument is for strong-named vs. weak-named assemblies
	echo.
	echo To build for the 1.1 framework, execute from a Visual Studio .Net 2003 command prompt.
	echo To build for the 2.0 framework, execute from a Visual Studio .Net 2005 command prompt.
goto :eof

:Compile_P4DN_Assembly
	
	rem set the source files
	set sources= "%SrcPath%\p4dn\AssemblyInfo.cpp"
	set sources= %sources% "%SrcPath%\p4dn\ClientApi_m.cpp"
	set sources= %sources% "%SrcPath%\p4dn\ClientUserDelegate.cpp"
	set sources= %sources% "%SrcPath%\p4dn\ClientUser_m.cpp"
	set sources= %sources% "%SrcPath%\p4dn\Error_m.cpp"
	set sources= %sources% "%SrcPath%\p4dn\NoEcho_m.cpp"
	set sources= %sources% "%SrcPath%\p4dn\Options_m.cpp"
	set sources= %sources% "%SrcPath%\p4dn\P4String.cpp"
	set sources= %sources% "%SrcPath%\p4dn\Spec_m.cpp"
	set sources= %sources% "%SrcPath%\p4dn\Stdafx.cpp"
	
	if '%CompilingSN%'=='true'  (
		set sources= %sources% ""%SrcPath%\p4dn\SNAssemblyInfo.cpp"
		copy /y "%BasePath%\build\P4.Net.snk" "%OutputPath%\P4.Net.snk" >nul
	)
	
	rem point to the P4API header files
	set FLAGS=/Od /I "%BasePath%\ext\p4api_vs2005_dyn\include\p4"
	
	rem VS compiler switches
	set FLAGS=%FLAGS% /D "WIN32" /D "OS_NT" /D "_WINDLL" /D "_MBCS" 
	
	rem P4API compiler switches
	set FLAGS=%FLAGS% /D "CASE_INSENSITIVE"
	set FLAGS=%FLAGS% /FD /EHa 
	set FLAGS=%FLAGS% /MD
	set FLAGS=%FLAGS% /W0 /nologo /c /Zi /TP
	
	rem different CLR flags for different versions
	if "%DotNetFWVersion%"=="2.0" (
		set FLAGS=%FLAGS% /clr:oldSyntax 
		
	)
	if "%DotNetFWVersion%"=="1.1" (
		set FLAGS=%FLAGS% /clr /d1PrivateNativeTypes 
	)
	
	rem now compile
	cl %FLAGS% %sources%
	
	rem set the output assembly
	set FLAGS=/OUT:"%OutputPath%\p4dn.dll" 
	
	
	set FLAGS=%FLAGS% /NOLOGO /DLL /NOENTRY
	rem set FLAGS=%FLAGS% /NODEFAULTLIB:"LIBCMT.lib" /NODEFAULTLIB:"LIBCMTD.lib" 
	set FLAGS=%FLAGS% /INCLUDE:"__DllMainCRTStartup@12" 
	set FLAGS=%FLAGS% /FIXED:No
	set FLAGS=%FLAGS% Ws2_32.lib
	set FLAGS=%FLAGS% msvcrt.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib 
	set FLAGS=%FLAGS% advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib
	
	rem Build against the 2003 version of P4API, otherwise a dependency on VS2005 SP1 VC runtime is 
	rem introduced... and that is not widely deployed on windows machines
	set FLAGS=%FLAGS% ..\ext\p4api_vs2003_dyn\lib\libclient.lib ..\ext\p4api_vs2003_dyn\lib\librpc.lib ..\ext\p4api_vs2003_dyn\lib\libsupp.lib 

	rem for 2.0 embed the manifest file
	if "%DotNetFWVersion%"=="2.0" (
		set FLAGS=%FLAGS% /MANIFEST /MANIFESTFILE:"%OutputPath%\p4dn.dll.intermediate.manifest"	
	)

		

	rem and now we link...
	link %FLAGS% "*.obj"
	
	rem for 2.0 embed the manifest file
	if "%DotNetFWVersion%"=="2.0" (
		mt /nologo /outputresource:"%OutputPath%\p4dn.dll;#2" /manifest "%OutputPath%\p4dn.dll.intermediate.manifest"
		rem del /q "%OutputPath%\p4dn.dll.intermediate.manifest"
	)
	
	if '%CompilingSN%'=='true'  (
		sn -q -R "%OutputPath%\p4dn.dll" "%BasePath%\build\P4.Net.snk"
	)

	rem now clean-up
	del /q "*.obj"
	del /q "*.pdb"
	del /q "*.idb"
	

goto :eof

:Compile_P4API_Assembly
	set FLAGS=/target:library 
	set FLAGS=%FLAGS% /reference:"%OutputPath%\p4dn.dll" 
	set FLAGS=%FLAGS% /w:0 /o
	set FLAGS=%FLAGS% "/out:%OutputPath%\p4api.dll"
	set FLAGS=%FLAGS% /nologo "/doc:%OutputPath%\p4api.xml"
	
	set SOURCES="%SrcPath%\P4API\*.cs" "%SrcPath%\P4API\Exceptions\*.cs" "%SrcPath%\P4API\Record\*.cs"
	if '%CompilingSN%'=='true'  (
		set SOURCES= %SOURCES% "%SrcPath%\P4API\StrongName\SNAssemblyInfo.cs"
	)
	
	echo Compiling p4api.dll...
	csc %FLAGS% %SOURCES%

goto :eof