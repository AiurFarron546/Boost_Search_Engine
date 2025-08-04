@echo off

chcp 65001

echo ===== Boost搜索引擎编译脚本 =====

REM 清理旧的构建目录
if exist "build" (
    echo 清理旧的构建文件...
    rmdir /s /q build
)

REM 创建新的build目录
echo 创建build目录...
mkdir build
cd build

echo 配置CMake项目...
cmake .. -G "Visual Studio 17 2022" -A x64

if %ERRORLEVEL% neq 0 (
    echo CMake配置失败，请检查：
    echo 1. 是否安装了Visual Studio 2022或更高版本
    echo 2. 是否安装了CMake
    echo 3. 是否正确安装了Boost库
    pause
    exit /b 1
)

echo 编译项目...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo 编译失败，请检查错误信息
    pause
    exit /b 1
)

echo 编译成功！
echo 可执行文件位置: build\Release\BoostSearchEngine.exe
echo.
echo 运行程序请执行: run.bat
pause