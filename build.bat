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
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=D:/CodeBuddy/CPP_envs/boost_1_88_0 -DBOOST_LIBRARYDIR=D:/CodeBuddy/CPP_envs/boost_1_88_0/stage/lib -DBoost_NO_SYSTEM_PATHS=ON -DBoost_USE_STATIC_LIBS=ON

if %ERRORLEVEL% neq 0 (
    echo CMake配置失败，请检查：
    echo 1. 是否安装了MinGW-w64 8.1.0或更高版本
    echo 2. 是否安装了CMake
    echo 3. 是否正确安装了Boost库（路径：D:\CodeBuddy\CPP_envs\boost_1_88_0）
    echo 4. MinGW-w64是否已添加到系统PATH环境变量中
    pause
    exit /b 1
)

echo 编译项目...
cmake --build . --parallel

if %ERRORLEVEL% neq 0 (
    echo 编译失败，请检查错误信息
    pause
    exit /b 1
)

echo.
echo 编译成功！
echo.
echo 可执行文件位置: build\BoostSearchEngine.exe
echo.
echo 运行程序请执行: run.bat
pause