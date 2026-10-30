@echo off
setlocal
cd /d "%~dp0"
set "ST_SERIAL_PY=D:\Complie\Miniconda\python.exe"
if exist "%ST_SERIAL_PY%" goto launch
set "ST_SERIAL_PY=python"
where python >nul 2>nul
if not errorlevel 1 goto launch
echo Python 3 is required. Please install Python 3, then run this file again.
pause
exit /b 1
:launch
echo ST Serial Console: http://127.0.0.1:8765/
echo Use desktop Chrome or Edge. Close other serial terminal programs first.
echo Keep this window open. Press Ctrl+C to stop the local server.
"%ST_SERIAL_PY%" -B -u -c "import http.server,webbrowser; server=http.server.ThreadingHTTPServer(('127.0.0.1',8765),http.server.SimpleHTTPRequestHandler); webbrowser.open('http://127.0.0.1:8765/'); server.serve_forever()"
if errorlevel 1 echo Server stopped or port 8765 is in use. See the message above.
pause
endlocal
