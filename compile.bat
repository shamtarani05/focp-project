@echo off
echo ============================================
echo   National Bank - Banking System Builder
echo   (GUI Desktop Application)
echo ============================================
echo.

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] g++ not found. Please install MinGW-w64.
    pause
    exit /b 1
)

echo Compiling GUI application...
echo.

g++ -std=c++14 -Isrc -o build\banking.exe ^
    src\main.cpp ^
    src\banking.cpp ^
    src\core\fileio.cpp ^
    src\core\validation.cpp ^
    -mwindows -lgdi32 -luser32 -lcomctl32

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo.
echo ============================================
echo   Build successful!
echo   Output: build\banking.exe
echo ============================================
echo.

if not exist data mkdir data
if not exist receipts mkdir receipts
if not exist logs mkdir logs
if not exist backup mkdir backup

echo Launching banking system...
echo.
cd build
start banking.exe
cd ..
