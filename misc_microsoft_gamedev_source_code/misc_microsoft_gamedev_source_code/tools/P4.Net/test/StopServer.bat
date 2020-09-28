@echo off

set ScriptPath=%~p0

rem make sure we have a p4d.exe
if not exist "%CD%\p4d.exe" (
	echo "Must have p4d.exe in the current directory to run tests!"
	goto :eof
)

rem make sure we have a p4.exe
if not exist "%CD%\p4.exe" (
	echo "Must have p4.exe in the current directory to run tests!"
	goto :eof
)

set P4DPORT=5791

set P4Dexe=%ScriptPath%\p4d.exe
set P4exe=%ScriptPath%\p4.exe

echo.
echo Stopping Perforce Server
echo.

%P4exe% -p %P4DPORT% -u TestUser2 admin stop >nul 2>nul

