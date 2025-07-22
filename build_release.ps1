# MistEngine Release Build Script (PowerShell)
# Builds the engine for release distribution

Write-Host ""
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "   MistEngine Pre-Alpha Build" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

# Check if MSBuild is available
if (-not (Get-Command msbuild -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: MSBuild not found in PATH" -ForegroundColor Red
    Write-Host "Please run from Visual Studio Developer PowerShell" -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Configuration settings
$SOLUTION = "MistEngine.sln"
$CONFIG = "Release"
$PLATFORM = "x64"
$BUILD_DIR = "build\release"

Write-Host "Building $SOLUTION..." -ForegroundColor Green
Write-Host "Configuration: $CONFIG" -ForegroundColor White
Write-Host "Platform: $PLATFORM" -ForegroundColor White
Write-Host ""

# Clean previous builds
if (Test-Path $BUILD_DIR) {
    Write-Host "Cleaning previous build..." -ForegroundColor Yellow
    Remove-Item $BUILD_DIR -Recurse -Force
}

# Create build directory structure
Write-Host "Creating build structure..." -ForegroundColor Yellow
New-Item -Path "$BUILD_DIR\bin" -ItemType Directory -Force | Out-Null
New-Item -Path "$BUILD_DIR\shaders" -ItemType Directory -Force | Out-Null
New-Item -Path "$BUILD_DIR\models" -ItemType Directory -Force | Out-Null
New-Item -Path "$BUILD_DIR\textures" -ItemType Directory -Force | Out-Null
New-Item -Path "$BUILD_DIR\docs" -ItemType Directory -Force | Out-Null

# Build the solution
Write-Host "Building solution..." -ForegroundColor Green
$buildResult = Start-Process -FilePath "msbuild" -ArgumentList "$SOLUTION", "/p:Configuration=$CONFIG", "/p:Platform=$PLATFORM", "/m", "/v:minimal" -Wait -PassThru -NoNewWindow

if ($buildResult.ExitCode -ne 0) {
    Write-Host ""
    Write-Host "? BUILD FAILED!" -ForegroundColor Red
    Write-Host "Check the output above for errors." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Copy release files
Write-Host ""
Write-Host "Copying release files..." -ForegroundColor Green

# Copy executable and dependencies
$exePath = "x64\$CONFIG\MistEngine.exe"
if (Test-Path $exePath) {
    Copy-Item $exePath "$BUILD_DIR\bin\" -Force
    Write-Host "? Copied MistEngine.exe" -ForegroundColor Green
} else {
    Write-Host "? MistEngine.exe not found!" -ForegroundColor Red
    exit 1
}

# Copy DLLs if they exist
Get-ChildItem "x64\$CONFIG\*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item $_.FullName "$BUILD_DIR\bin\" -Force
    Write-Host "? Copied $($_.Name)" -ForegroundColor Green
}

# Copy assets
if (Test-Path "shaders") {
    Get-ChildItem "shaders\*.*" | Copy-Item -Destination "$BUILD_DIR\shaders\" -Force
    Write-Host "? Copied shaders" -ForegroundColor Green
}

if (Test-Path "models") {
    Get-ChildItem "models\*.*" | Copy-Item -Destination "$BUILD_DIR\models\" -Force
    Write-Host "? Copied models" -ForegroundColor Green
}

if (Test-Path "textures") {
    Get-ChildItem "textures\*.*" | Copy-Item -Destination "$BUILD_DIR\textures\" -Force
    Write-Host "? Copied textures" -ForegroundColor Green
}

# Copy documentation
$docs = @("README.md", "AI_README.md", "RELEASE_NOTES.md", "CONTRIBUTING.md")
foreach ($doc in $docs) {
    if (Test-Path $doc) {
        Copy-Item $doc "$BUILD_DIR\docs\" -Force
        Write-Host "? Copied $doc" -ForegroundColor Green
    }
}

if (Test-Path "LICENSE") {
    Copy-Item "LICENSE" "$BUILD_DIR\docs\" -Force
    Write-Host "? Copied LICENSE" -ForegroundColor Green
}

if (Test-Path "ai_config.example.json") {
    Copy-Item "ai_config.example.json" "$BUILD_DIR\" -Force
    Write-Host "? Copied ai_config.example.json" -ForegroundColor Green
}

# Create launch script
$launcherContent = @"
@echo off
:: MistEngine Launcher
cd /d "%~dp0"
bin\MistEngine.exe
pause
"@
$launcherContent | Out-File -FilePath "$BUILD_DIR\Launch_MistEngine.bat" -Encoding ASCII

# Create quick start guide
$quickStartContent = @"
# MistEngine Pre-Alpha - Quick Start

## Running the Engine
1. Run `Launch_MistEngine.bat` or
2. Navigate to `bin/` folder and run `MistEngine.exe`

## AI Assistant Setup (Optional)
1. Copy `ai_config.example.json` to `ai_config.json`
2. Get API key from: https://aistudio.google.com/app/apikey
3. Edit `ai_config.json` and add your key
4. In engine: AI Menu > Configure API Key

## Controls
- WASD: Move camera
- Mouse: Look around
- F2: Open AI assistant
- Right-click: Context menus

## Creating Objects
- GameObject Menu > 3D Object > Cube/Sphere/Plane
- Use Hierarchy window to manage objects
- Use Inspector to edit properties

## Documentation
See `docs/` folder for complete documentation

## Support
- GitHub: https://github.com/yourusername/MistEngine
- Issues: Report bugs on GitHub Issues
- AI Help: Press F2 in the engine

**Status: Pre-Alpha** - Expect bugs and missing features!
"@
$quickStartContent | Out-File -FilePath "$BUILD_DIR\QUICK_START.md" -Encoding UTF8

Write-Host ""
Write-Host "? BUILD SUCCESSFUL!" -ForegroundColor Green
Write-Host ""
Write-Host "Release files created in: $BUILD_DIR\" -ForegroundColor Cyan
Write-Host ""
Write-Host "?? Release Structure:" -ForegroundColor Yellow
Write-Host "  bin\              - Executable and DLLs" -ForegroundColor White
Write-Host "  shaders\          - Shader files" -ForegroundColor White
Write-Host "  models\           - 3D models" -ForegroundColor White
Write-Host "  textures\         - Texture files" -ForegroundColor White
Write-Host "  docs\             - Documentation" -ForegroundColor White
Write-Host "  ai_config.example.json - AI configuration template" -ForegroundColor White
Write-Host "  Launch_MistEngine.bat  - Easy launcher" -ForegroundColor White
Write-Host "  QUICK_START.md    - Quick start guide" -ForegroundColor White
Write-Host ""

# Test the executable
$exeFile = Get-Item "$BUILD_DIR\bin\MistEngine.exe"
$fileSizeMB = [math]::Round($exeFile.Length / 1MB, 2)
Write-Host "?? Executable Info:" -ForegroundColor Cyan
Write-Host "  File: MistEngine.exe" -ForegroundColor White
Write-Host "  Size: $fileSizeMB MB" -ForegroundColor White
Write-Host "  Ready to run!" -ForegroundColor Green

# Optional: Create zip file
Write-Host ""
$createZip = Read-Host "Create ZIP file for release? (y/n)"
if ($createZip -eq "y" -or $createZip -eq "Y") {
    Write-Host ""
    Write-Host "Creating release ZIP..." -ForegroundColor Yellow
    
    $zipName = "MistEngine-v0.1.0-pre-alpha-win64.zip"
    
    # Try using 7-Zip first
    if (Get-Command 7z -ErrorAction SilentlyContinue) {
        & 7z a -tzip $zipName ".\$BUILD_DIR\*"
        Write-Host "? ZIP created with 7-Zip: $zipName" -ForegroundColor Green
    }
    # Try using PowerShell Compress-Archive
    elseif (Get-Command Compress-Archive -ErrorAction SilentlyContinue) {
        Compress-Archive -Path "$BUILD_DIR\*" -DestinationPath $zipName -Force
        Write-Host "? ZIP created with PowerShell: $zipName" -ForegroundColor Green
    }
    else {
        Write-Host "??  No ZIP tool found. Please manually zip the $BUILD_DIR folder." -ForegroundColor Yellow
        Write-Host "   Suggested filename: $zipName" -ForegroundColor White
    }
}

Write-Host ""
Write-Host "?? Ready for GitHub release!" -ForegroundColor Green
Write-Host ""
Read-Host "Press Enter to exit"