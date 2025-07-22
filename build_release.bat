@echo off
:: MistEngine Build Script
:: Builds the engine for release distribution

setlocal enabledelayedexpansion

echo.
echo =====================================
echo    MistEngine Pre-Alpha Build
echo =====================================
echo.

:: Check if Visual Studio is available
where msbuild >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: MSBuild not found in PATH
    echo Please run from Visual Studio Developer Command Prompt
    echo Or install Visual Studio Build Tools
    pause
    exit /b 1
)

:: Configuration settings
set SOLUTION=MistEngine.sln
set CONFIG=Release
set PLATFORM=x64
set BUILD_DIR=build\release

echo Building %SOLUTION%...
echo Configuration: %CONFIG%
echo Platform: %PLATFORM%
echo.

:: Clean previous builds
if exist "%BUILD_DIR%" (
    echo Cleaning previous build...
    rmdir /s /q "%BUILD_DIR%"
)

:: Create build directory
mkdir "%BUILD_DIR%" 2>nul

:: Build the solution
echo Building solution...
msbuild "%SOLUTION%" /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m /v:minimal

if %errorlevel% neq 0 (
    echo.
    echo ? BUILD FAILED!
    echo Check the output above for errors.
    pause
    exit /b 1
)

:: Copy release files
echo.
echo Copying release files...

:: Create release structure
mkdir "%BUILD_DIR%\bin" 2>nul
mkdir "%BUILD_DIR%\shaders" 2>nul
mkdir "%BUILD_DIR%\models" 2>nul
mkdir "%BUILD_DIR%\textures" 2>nul
mkdir "%BUILD_DIR%\docs" 2>nul

:: Copy executable and dependencies
copy "x64\%CONFIG%\MistEngine.exe" "%BUILD_DIR%\bin\" >nul
if exist "x64\%CONFIG%\*.dll" (
    copy "x64\%CONFIG%\*.dll" "%BUILD_DIR%\bin\" >nul
)

:: Copy assets
copy "shaders\*.*" "%BUILD_DIR%\shaders\" >nul 2>nul
copy "models\*.*" "%BUILD_DIR%\models\" >nul 2>nul
copy "textures\*.*" "%BUILD_DIR%\textures\" >nul 2>nul

:: Copy documentation
copy "README.md" "%BUILD_DIR%\docs\" >nul
copy "AI_README.md" "%BUILD_DIR%\docs\" >nul
copy "RELEASE_NOTES.md" "%BUILD_DIR%\docs\" >nul
copy "CONTRIBUTING.md" "%BUILD_DIR%\docs\" >nul
copy "LICENSE" "%BUILD_DIR%\docs\" >nul 2>nul
copy "ai_config.example.json" "%BUILD_DIR%\" >nul

:: Create launch script
echo @echo off > "%BUILD_DIR%\Launch_MistEngine.bat"
echo :: MistEngine Launcher >> "%BUILD_DIR%\Launch_MistEngine.bat"
echo cd /d "%%~dp0" >> "%BUILD_DIR%\Launch_MistEngine.bat"
echo bin\MistEngine.exe >> "%BUILD_DIR%\Launch_MistEngine.bat"
echo pause >> "%BUILD_DIR%\Launch_MistEngine.bat"

:: Create quick start guide
(
echo # MistEngine Pre-Alpha - Quick Start
echo.
echo ## Running the Engine
echo 1. Run `Launch_MistEngine.bat` or
echo 2. Navigate to `bin/` folder and run `MistEngine.exe`
echo.
echo ## AI Assistant Setup ^(Optional^)
echo 1. Copy `ai_config.example.json` to `ai_config.json`
echo 2. Get API key from: https://aistudio.google.com/app/apikey
echo 3. Edit `ai_config.json` and add your key
echo 4. In engine: AI Menu ^> Configure API Key
echo.
echo ## Controls
echo - WASD: Move camera
echo - Mouse: Look around
echo - F2: Open AI assistant
echo - Right-click: Context menus
echo.
echo ## Creating Objects
echo - GameObject Menu ^> 3D Object ^> Cube/Sphere/Plane
echo - Use Hierarchy window to manage objects
echo - Use Inspector to edit properties
echo.
echo ## Documentation
echo See `docs/` folder for complete documentation
echo.
echo ## Support
echo - GitHub: https://github.com/yourusername/MistEngine
echo - Issues: Report bugs on GitHub Issues
echo - AI Help: Press F2 in the engine
echo.
echo **Status: Pre-Alpha** - Expect bugs and missing features!
) > "%BUILD_DIR%\QUICK_START.md"

echo.
echo ? BUILD SUCCESSFUL!
echo.
echo Release files created in: %BUILD_DIR%\
echo.
echo ?? Release Structure:
echo   bin\              - Executable and DLLs
echo   shaders\          - Shader files  
echo   models\           - 3D models
echo   textures\         - Texture files
echo   docs\             - Documentation
echo   ai_config.example.json - AI configuration template
echo   Launch_MistEngine.bat  - Easy launcher
echo   QUICK_START.md    - Quick start guide
echo.
echo Ready to zip and upload to GitHub! ??
echo.

:: Test the executable
echo Testing the executable...
if exist "%BUILD_DIR%\bin\MistEngine.exe" (
    echo ? MistEngine.exe found (Ready to run^)
    
    :: Get file size
    for %%A in ("%BUILD_DIR%\bin\MistEngine.exe") do (
        set "filesize=%%~zA"
    )
    echo    Size: !filesize! bytes
) else (
    echo ? MistEngine.exe not found in release folder!
    pause
    exit /b 1
)

:: Optional: Create zip file
set /p CREATE_ZIP="Create ZIP file for release? (y/n): "
if /i "%CREATE_ZIP%"=="y" (
    echo.
    echo Creating release ZIP...
    
    :: Check if 7zip is available
    where 7z >nul 2>nul
    if %errorlevel% equ 0 (
        7z a -tzip "MistEngine-v0.1.0-pre-alpha-win64.zip" ".\%BUILD_DIR%\*"
        echo ? ZIP created: MistEngine-v0.1.0-pre-alpha-win64.zip
    ) else (
        echo 7zip not found. Please manually zip the %BUILD_DIR% folder.
        echo Suggested filename: MistEngine-v0.1.0-pre-alpha-win64.zip
    )
)

echo.
echo ?? Ready for GitHub release!
echo.
pause