/**
 * @file main.cpp
 * @brief Boost搜索引擎的主程序入口
 *
 * 该文件包含程序的主函数`main`，负责整个应用程序的生命周期管理，
 * 包括初始化搜索引擎、启动HTTP服务器以及最后的资源清理。
 */

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "http_server.h"
#include "search_engine.h"
#include "indexer.h"

/**
 * @brief 全局搜索引擎实例指针
 *
 * 该指针在程序启动时被初始化，并在整个程序运行期间提供对搜索引擎的访问。
 * 其他模块通过`get_search_engine()`函数来获取此实例。
 */
SearchEngine* g_search_engine = nullptr;

/**
 * @brief 初始化全局搜索引擎实例
 *
 * 该函数负责创建`SearchEngine`对象，加载数据文件，并构建索引。
 * @return 如果初始化成功返回`true`，否则返回`false`。
 */
bool initialize_search_engine() {
    try {
        std::cout << "Initializing search engine..." << std::endl;

        // 1. 创建搜索引擎实例
        g_search_engine = new SearchEngine();

        // 2. 从指定目录加载数据文件
        g_search_engine->load_data_files("./data");

        // 3. 根据加载的文档构建搜索引擎索引
        g_search_engine->build_index();

        std::cout << "Search engine initialization completed!" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Search engine initialization failed: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 清理程序资源
 *
 * 在程序退出前，释放全局搜索引擎实例所占用的内存。
 */
void cleanup() {
    if (g_search_engine) {
        delete g_search_engine;
        g_search_engine = nullptr;
        std::cout << "Search engine resources cleaned up." << std::endl;
    }
}

/**
 * @brief 程序主函数
 *
 * @return 程序退出码，0表示成功，非0表示失败。
 */
int main() {
    try {
        std::cout << "=== Boost Search Engine Starting ===" << std::endl;

        // 初始化搜索引擎，如果失败则退出程序
        if (!initialize_search_engine()) {
            return 1;
        }

        // 创建Boost.Asio的I/O服务，用于网络操作
        boost::asio::io_service io_service;

        // 创建并启动HTTP服务器，监听指定端口（9882）
        HttpServer server(io_service, 9882);

        std::cout << "HTTP server started, listening on port: 9882" << std::endl;
        std::cout << "Please visit: http://localhost:9882" << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;

        // 运行I/O服务，开始处理异步事件（如HTTP请求）
        // 此调用会阻塞，直到io_service停止
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Program exception: " << e.what() << std::endl;
    }

    // 程序结束前，执行资源清理
    cleanup();

    std::cout << "=== Boost Search Engine Stopped ===" << std::endl;
    return 0;
}

/**
 * @brief 获取全局搜索引擎实例的访问接口
 *
 * @return 指向全局`SearchEngine`实例的指针。
 *
 * 该函数允许其他模块（如http_server）访问唯一的搜索引擎实例。
 */
SearchEngine* get_search_engine() {
    return g_search_engine;
}
