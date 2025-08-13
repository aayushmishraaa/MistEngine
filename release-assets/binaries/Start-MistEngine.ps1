# MistEngine Launcher Script
Write-Host "Starting MistEngine v0.3.0" -ForegroundColor Green
Write-Host "Built with modern C++14 and AI integration" -ForegroundColor Cyan
Write-Host ""
if (Test-Path "MistEngine.exe") {
    Write-Host "Starting MistEngine..." -ForegroundColor Yellow
    Start-Process -FilePath ".\MistEngine.exe" -Wait
} else {
    Write-Host "MistEngine.exe not found" -ForegroundColor Red
    Write-Host "Make sure you're running this from the binaries directory." -ForegroundColor Red
}
Write-Host ""
Read-Host "Press Enter to exit"
