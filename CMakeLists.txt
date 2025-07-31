cmake_minimum_required(VERSION 3.16)
project(BoostSearchEngine)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器标志
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /utf-8")
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system filesystem thread regex)

# 包含头文件目录
include_directories(${Boost_INCLUDE_DIRS})
include_directories(include)

# 添加源文件
set(SOURCES
    src/main.cpp
    src/http_server.cpp
    src/search_engine.cpp
    src/indexer.cpp
    src/text_processor.cpp
)

set(HEADERS
    include/http_server.h
    include/search_engine.h
    include/indexer.h
    include/text_processor.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 链接Boost库
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    ws2_32
    wsock32
)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)
