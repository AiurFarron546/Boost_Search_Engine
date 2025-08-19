cmake_minimum_required(VERSION 3.16)
project(BoostSearchEngine)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器标志和优化选项
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    # MSVC优化设置
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DBOOST_ALL_NO_LIB)
    add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
elseif(MINGW)
    # MinGW-w64特定配置
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")
    # MinGW优化设置
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DBOOST_ALL_NO_LIB)
    add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
    # 解决MinGW的一些兼容性问题
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    # 强制静态链接
    add_definitions(-DBOOST_FILESYSTEM_STATIC_LINK)
    add_definitions(-DBOOST_SYSTEM_STATIC_LINK)
else()
    # GCC/Clang优化设置
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
endif()

# 设置Boost路径和配置 - 使用MSYS2的UCRT64环境
set(BOOST_ROOT "C:/msys64/ucrt64")
set(BOOST_LIBRARYDIR "C:/msys64/ucrt64/lib")
set(Boost_INCLUDE_DIR "C:/msys64/ucrt64/include")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_NO_SYSTEM_PATHS ON)  # 强制使用指定路径

# 对于MinGW/MSYS2，使用传统的FindBoost模块
if(MINGW)
    # MSYS2 UCRT64环境
    set(Boost_DEBUG ON)
    # 添加可能的库名后缀
    set(Boost_LIB_PREFIX "lib")
    set(CMAKE_FIND_LIBRARY_PREFIXES "lib" "")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll.a" ".a" ".lib")
endif()

# 设置Boost查找路径
set(CMAKE_PREFIX_PATH "C:/msys64/ucrt64" ${CMAKE_PREFIX_PATH})

find_package(Boost REQUIRED COMPONENTS 
    system 
    filesystem 
    thread 
    regex
)

if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
    message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
else()
    message(FATAL_ERROR "Boost not found!")
endif()

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})

# 源文件
set(SOURCES
    src/main.cpp
    src/search_engine.cpp
    src/indexer.cpp
    src/text_processor.cpp
    src/http_server.cpp
)

# 头文件
set(HEADERS
    include/search_engine.h
    include/indexer.h
    include/text_processor.h
    include/http_server.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接库 - 使用传统方式链接Boost库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
)

# Windows特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
endif()

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)

# 复制数据文件到构建目录
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/web DESTINATION ${CMAKE_BINARY_DIR})

# 安装规则
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY data/
    DESTINATION bin/data
)

install(DIRECTORY web/
    DESTINATION bin/web
)