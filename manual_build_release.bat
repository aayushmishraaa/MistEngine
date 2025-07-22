@echo off
:: Manual Release Build Script for MistEngine
:: This script builds the project manually with correct include paths

echo.
echo ===================================
echo   MistEngine Manual Release Build
echo ===================================
echo.

:: Set paths
set INCLUDE_PATHS=/I"D:\Libraries\bullet3_x64-windows\include" /I"D:\Libraries\Assimp\include" /I"D:\Libraries\freetype-2.13.3\include" /I"D:\Libraries\freeglut-3.6.0\include" /I"E:\Projects\MistEngine\include" /I"D:\Libraries\glm-master" /I"D:\Libraries\glfw-3.4.bin.WIN64\include" /I"D:\Libraries\glad\include"
set LIB_PATHS=/LIBPATH:"D:\Libraries\bullet3_x64-windows\lib" /LIBPATH:"D:\Libraries\Assimp\lib\x64" /LIBPATH:"D:\Libraries\glfw-3.4.bin.WIN64\lib-vc2022"
set LIBS=glfw3.lib opengl32.lib assimp-vc143-mt.lib BulletCollision.lib BulletDynamics.lib LinearMath.lib wininet.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib

:: Compiler flags
set CFLAGS=/O2 /MD /W3 /EHsc /DNDEBUG /D_CONSOLE /std:c++14
set LFLAGS=/MACHINE:X64 /SUBSYSTEM:CONSOLE

:: Create output directory
if not exist "x64\Release" mkdir "x64\Release"

echo Compiling C++ source files...

:: Compile all cpp files
cl %CFLAGS% %INCLUDE_PATHS% /c src\*.cpp /Fo"x64\Release\"
if %errorlevel% neq 0 goto :error

echo Compiling AI module...
cl %CFLAGS% %INCLUDE_PATHS% /c src\AI\*.cpp /Fo"x64\Release\"
if %errorlevel% neq 0 goto :error

echo Compiling ECS Systems...
cl %CFLAGS% %INCLUDE_PATHS% /c src\ECS\Systems\*.cpp /Fo"x64\Release\"
if %errorlevel% neq 0 goto :error

echo Compiling C source files...
cl /O2 /MD %INCLUDE_PATHS% /c src\glad.c /Fo"x64\Release\glad.obj"
if %errorlevel% neq 0 goto :error

echo Linking executable...
link %LFLAGS% x64\Release\*.obj %LIB_PATHS% %LIBS% /OUT:"x64\Release\MistEngine.exe"
if %errorlevel% neq 0 goto :error

echo.
echo ? BUILD SUCCESSFUL!
echo Executable: x64\Release\MistEngine.exe
echo.
goto :end

:error
echo.
echo ? BUILD FAILED!
echo Check the output above for errors.
echo.
exit /b 1

:end
echo Build completed successfully.
pause