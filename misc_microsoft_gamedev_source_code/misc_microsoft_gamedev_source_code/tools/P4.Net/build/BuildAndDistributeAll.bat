@echo off

rem Build everything and stage for Perforce.  Not really usefull, unless you
rem are updating a new version in the public depot.


rem revert first (allows script to be re-run before submit)
p4 revert "..\bin\..."
p4 revert "..\doc\chm\*"
p4 revert "..\doc\html\..."

rem open for edit files we know will be updated 
p4 edit ..\bin\...
p4 edit "..\doc\chm\*"

rem launch each combination for the binaries
echo @echo off >tmp.bat
echo call "%ProgramFiles%\Microsoft Visual Studio 8\VC\vcvarsall.bat" x86 >>tmp.bat
echo call "%CD%\compile_p4dn.bat" 2.0 >>tmp.bat
 
start /w %comspec% /c "%cd%\tmp.bat"
 
echo @echo off >tmp.bat
echo call "%ProgramFiles%\Microsoft Visual Studio 8\VC\vcvarsall.bat" x86 >>tmp.bat
echo call "%CD%\compile_p4dn.bat" 2.0 SN >>tmp.bat
 
start /w %comspec% /c "%cd%\tmp.bat"
 

echo @echo off >tmp.bat
echo call "%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat" >>tmp.bat
echo call "%CD%\compile_p4dn.bat" 1.1 >>tmp.bat
 
start /w %comspec% /c "%cd%\tmp.bat"
 

echo @echo off >tmp.bat
echo call "%ProgramFiles%\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat" >>tmp.bat
echo call "%CD%\compile_p4dn.bat" 1.1 SN >>tmp.bat
 
start /w %comspec% /c "%cd%\tmp.bat"
 
del "%cd%\tmp.bat" >nul

rem build the documentation
pushd ..\doc
attrib -r -s html\*
call builddoc.bat
call :updateP4 html

popd

rem revert any unchanged files just in case
p4 revert -a //...


goto :eof



rem working offline technique
:updateP4

	pushd %~1
	p4 diff -se ... | p4 -x - edit
	p4 diff -sd ... | p4 -x - delete
	dir /b /s /a:-d | p4 -x - add
	popd

goto :eof