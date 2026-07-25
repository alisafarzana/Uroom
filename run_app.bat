@echo off
cd /d "%~dp0"
where g++ >nul 2>nul
if errorlevel 1 (
  echo ERROR: g++ was not found. Install MinGW-w64 or MSYS2 and add it to PATH.
  pause
  exit /b 1
)
echo Compiling C++ backend...
g++ backend.cpp -std=c++17 -O2 -o backend.exe
if errorlevel 1 (pause & exit /b 1)
echo Installing Python requirements...
python -m pip install -r requirements.txt
if errorlevel 1 (pause & exit /b 1)
echo Starting Streamlit...
python -m streamlit run app.py
pause
