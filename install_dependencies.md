# Boost搜索引擎 - 依赖安装指南

## Windows 10 环境配置

### 1. 安装Visual Studio

1. 下载Visual Studio Community 2019或2022（免费版本）
   - 官网：https://visualstudio.microsoft.com/zh-hans/downloads/
   
2. 安装时选择以下组件：
   - C++桌面开发工作负载
   - Windows 10 SDK
   - CMake工具
   - Git for Windows

### 2. 安装CMake

1. 下载CMake最新版本
   - 官网：https://cmake.org/download/
   
2. 安装时选择"Add CMake to system PATH"

### 3. 安装Boost库

#### 方法一：使用vcpkg（推荐）

1. 安装vcpkg包管理器：
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

2. 安装Boost库：
```bash
.\vcpkg install boost:x64-windows
```

3. 集成到Visual Studio：
```bash
.\vcpkg integrate install
```

#### 方法二：手动编译Boost

1. 下载Boost源码
   - 官网：https://www.boost.org/users/download/
   
2. 解压到C:\boost目录

3. 编译Boost：
```bash
cd C:\boost
.\bootstrap.bat
.\b2 --build-type=complete --with-system --with-filesystem --with-thread --with-regex toolset=msvc architecture=x86 address-model=64
```

### 4. 环境变量配置

如果使用手动编译的Boost，需要设置环境变量：

1. 添加系统环境变量：
   - `BOOST_ROOT` = `C:\boost`
   - `BOOST_LIBRARYDIR` = `C:\boost\stage\lib`

2. 将以下路径添加到PATH：
   - `C:\boost\stage\lib`

### 5. 验证安装

1. 打开命令提示符，运行：
```bash
cmake --version
```

2. 检查Visual Studio是否正确安装C++编译器

### 6. 编译项目

1. 克隆或下载项目到本地
2. 打开命令提示符，进入项目目录
3. 运行编译脚本：
```bash
build.bat
```

### 7. 运行项目

编译成功后，运行：
```bash
run.bat
```

然后在浏览器中访问：http://localhost:8080

## 常见问题解决

### 问题1：CMake找不到Boost库
**解决方案：**
- 确保Boost库正确安装
- 检查环境变量设置
- 使用vcpkg安装Boost（推荐）

### 问题2：编译错误 - 缺少头文件
**解决方案：**
- 检查Visual Studio是否安装了C++开发工具
- 确保Windows SDK已安装

### 问题3：链接错误
**解决方案：**
- 检查Boost库是否为64位版本
- 确保编译器架构匹配（x64）

### 问题4：运行时找不到DLL
**解决方案：**
- 将Boost库的bin目录添加到PATH
- 或者将所需DLL复制到可执行文件目录

### 问题5：端口8080被占用
**解决方案：**
- 修改src/main.cpp中的端口号
- 或者停止占用8080端口的其他程序

## 性能优化建议

1. **Release模式编译**：确保使用Release配置编译，获得最佳性能
2. **内存设置**：根据数据量调整索引缓存大小
3. **线程数量**：根据CPU核心数调整工作线程数量
4. **文件系统**：使用SSD存储可提高文件读取速度

## 开发环境推荐

- **IDE**：Visual Studio 2019/2022 或 Visual Studio Code
- **调试工具**：Visual Studio Debugger
- **性能分析**：Visual Studio Diagnostic Tools
- **版本控制**：Git
- **文档工具**：Doxygen（可选）