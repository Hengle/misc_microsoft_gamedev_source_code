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

rem just in case the server is running...
%P4exe% -p %P4DPORT% admin stop >nul 2>nul

if exist "%ScriptPath%\p4root" rd /s /q "%ScriptPath%\p4root"
mkdir "%ScriptPath%\p4root"

echo.
echo Launching Perforce Server
echo.

start /MIN cmd /c %P4Dexe% -r %CD%\p4root -p %P4DPORT% -J off -vdebug=3

rem enable monitor and restart
p4 -p %P4DPORT% -u TestUser2 counter -f monitor 1

p4 -p %P4DPORT% -u TestUser2 admin stop >nul 2>nul

if "%~1"=="unicode" %P4Dexe% -r %CD%\p4root -p %P4DPORT% -xi

start /MIN cmd /c %P4Dexe% -r %CD%\p4root -p %P4DPORT% -J off -vdebug=3
