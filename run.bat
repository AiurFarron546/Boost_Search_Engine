@echo off

chcp 65001

echo ===== 启动Boost搜索引擎 =====

REM 检查可执行文件是否存在
if not exist "build\Release\BoostSearchEngine.exe" (
    echo 可执行文件不存在，请先运行 build.bat 编译项目
    pause
    exit /b 1
)

echo 启动搜索引擎服务器...

REM 启动程序
build\Release\BoostSearchEngine.exe

echo 服务器将在 http://localhost:9882 启动
echo 按 Ctrl+C 停止服务器
echo.

pause
