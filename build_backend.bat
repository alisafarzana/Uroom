@echo off
cd /d "%~dp0"
g++ backend.cpp -std=c++17 -O2 -o backend.exe
if errorlevel 1 (echo Compilation failed. & pause & exit /b 1)
echo backend.exe created successfully.
pause
