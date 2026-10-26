@echo off
rem Builds the emulator using the makefile in bin\
rem Usage: compile.bat          build ASM.exe
rem        compile.bat test     build and run the tests
rem        compile.bat clean    remove build outputs
echo Compiling project using Makefile...

rem Go to the bin folder next to this script, wherever it is run from
pushd "%~dp0bin" || (
    echo Could not find the bin folder next to compile.bat
    pause
    exit /b 1
)

mingw32-make -f makefile %*
set result=%errorlevel%
popd

if %result% neq 0 (
    echo Compilation failed!
    pause
    exit /b %result%
)
echo Compilation successful!
