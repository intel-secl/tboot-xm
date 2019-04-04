@echo off
setlocal enabledelayedexpansion

set me=%~n0
set pwd=%~dp0
set "tbootxm_home=%pwd%\..\..\..\"
IF "%NSIS_HOME%"=="" (
  set "makensis=C:\Program Files (x86)\NSIS\makensis.exe"
) ELSE (
  set "makensis=%NSIS_HOME%\makensis.exe"
)

IF "%~1"=="" (
  call:print_help
) ELSE IF "%~2"=="" (
  call:print_help
) ELSE IF "%~3"=="" (
  call:print_help
) ELSE (
  call:tbootxm_install %1 %2 %3
)
GOTO:EOF

:tbootxm_install
  echo. Creating tbootxm installer.... %1 %2 %3
  REM call:tbootxm_build %1 %2 %3
  call:tbootxm_install
GOTO:EOF

:tbootxm_build
  echo. Building tbootxm_driver.... %1 %2 %3
  cd %tbootxm_home%
  cd
  call "%tbootxm_home%\windows_bootdriver\tbootxm_bootdriver_build.cmd" %1 "%3 %2"
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: tbootxm build failed
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
GOTO:EOF

:tbootxm_install
  echo. Creating tbootxm installer....
  cd %pwd%
  cd
  IF EXIST "%makensis%" (
    echo. "%makensis% exists"
    call "%makensis%" tbootxm_installer.nsi
	) ELSE (
	echo. "Neither makensis.exe found at default location nor environment variable pointing to NSIS_HOME exist."
	echo. "If NSIS not installed please install it and add NSIS_HOME environment variable in system variables"
	call:ExitBatch
	REM EXIT /b 1
  )
  IF NOT %ERRORLEVEL% EQU 0 (
    echo. %me%: tbootxm install failed
	call:ExitBatch
	REM EXIT /b %ERRORLEVEL%
  )
GOTO:EOF

:print_help
  echo. "Usage: $0 Platform Configuration OS"
GOTO:EOF

:ExitBatch - Cleanly exit batch processing, regardless how many CALLs
if not exist "%temp%\ExitBatchYes.txt" call :buildYes
call :CtrlC <"%temp%\ExitBatchYes.txt" 1>nul 2>&1
:CtrlC
cmd /c exit -1073741510

:buildYes - Establish a Yes file for the language used by the OS
pushd "%temp%"
set "yes="
copy nul ExitBatchYes.txt >nul
for /f "delims=(/ tokens=2" %%Y in (
  '"copy /-y nul ExitBatchYes.txt <nul"'
) do if not defined yes set "yes=%%Y"
echo %yes%>ExitBatchYes.txt
popd
exit /b

endlocal