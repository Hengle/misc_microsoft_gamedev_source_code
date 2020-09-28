echo on
REM Build root is the base directory of the build environment
SET BUILDROOT=c:\a

REM Get rid of old log files
c:
del /f /Q %buildroot%\logs\*.*
del /f /Q %buildroot%\*.*
del /F /Q %buildroot%\build.zip


