@echo off
echo Starting MistEngine v0.3.0
echo Built with modern C++14 and AI integration
echo.
if exist MistEngine.exe (
    echo Starting MistEngine...
    MistEngine.exe
) else (
    echo MistEngine.exe not found
    echo Make sure you're running this from the binaries directory.
)
echo.
echo Press any key to exit...
