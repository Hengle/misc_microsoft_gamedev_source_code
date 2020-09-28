p4 set P4PORT=essd1:1666
@IF %ERRORLEVEL% NEQ 0 goto errorExit
p4 set P4USER=%1
@IF %ERRORLEVEL% NEQ 0 goto errorExit
p4 set P4CLIENT=%2
@IF %ERRORLEVEL% NEQ 0 goto errorExit
p4 logout
ECHO Password> temp_password.txt
p4 login < temp_password.txt
@IF %ERRORLEVEL% NEQ 0 goto errorExit
del temp_password.txt
goto normalExit

:errorExit
@echo off
echo Perforce login error
exit 1

:normalExit
exit 0
