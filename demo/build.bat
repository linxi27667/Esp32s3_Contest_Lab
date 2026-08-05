@echo off
setlocal
cd /d "%~dp0"

if "%IDF_PATH%"=="" (
    echo IDF_PATH is not set. Open an ESP-IDF terminal or run export.bat first.
    exit /b 1
)

call "%IDF_PATH%\export.bat"
idf.py build
