cmake_minimum_required(VERSION 3.16)
project(BoostSearchEngine)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器标志
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    add_definitions(-D_WIN32_WINNT=0x0601)
    add_definitions(-DBOOST_ALL_NO_LIB)
    add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)
endif()

# 查找Boost库
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED COMPONENTS 
    system 
    filesystem 
    thread 
    regex
    locale
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

# 链接库
target_link_libraries(${PROJECT_NAME} 
    ${Boost_LIBRARIES}
)

# Windows特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME} ws2_32 wsock32)
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