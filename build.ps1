# NOMAD Build Script
Write-Host "`n==================================" -ForegroundColor Cyan
Write-Host "  NOMAD DAW - Build System" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan

# Step 1: Clone FreeType if needed
$freetypeDir = "build\_deps\freetype-src"
if (-not (Test-Path $freetypeDir)) {
    Write-Host "`n[1/3] Cloning FreeType..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build\_deps" -Force | Out-Null
    git clone --depth 1 --branch VER-2-13-3 https://github.com/freetype/freetype.git $freetypeDir
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to clone FreeType!" -ForegroundColor Red
        exit 1
    }
    Write-Host "FreeType cloned!" -ForegroundColor Green
} else {
    Write-Host "`n[1/3] FreeType already exists" -ForegroundColor Green
}

# Step 2: Configure CMake
Write-Host "`n[2/3] Configuring CMake..." -ForegroundColor Yellow
cmake -B build -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) {
    Write-Host "Configuration failed!" -ForegroundColor Red
    exit 1
}
Write-Host "Configuration complete!" -ForegroundColor Green

# Step 3: Build NomadUI
Write-Host "`n[3/3] Building NomadUI..." -ForegroundColor Yellow
cmake --build build --config Release --target NomadUI_Core
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}
Write-Host "Build complete!" -ForegroundColor Green

Write-Host "`n==================================" -ForegroundColor Cyan
Write-Host "  Build Successful!" -ForegroundColor Green
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""
