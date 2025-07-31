/**
 * Boost搜索引擎 - 主程序入口
 *
 * 功能：
 * 1. 初始化搜索引擎
 * 2. 启动HTTP服务器
 * 3. 处理用户搜索请求
 */

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "http_server.h"
#include "search_engine.h"
#include "indexer.h"

// 全局搜索引擎实例
SearchEngine* g_search_engine = nullptr;

/**
 * 初始化搜索引擎
 */
bool initialize_search_engine() {
    try {
        std::cout << "Initializing search engine..." << std::endl;

        // 创建搜索引擎实例
        g_search_engine = new SearchEngine();

        // 加载数据文件
        g_search_engine->load_data_files("./data");

        // 构建索引
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
 * 清理资源
 */
void cleanup() {
    if (g_search_engine) {
        delete g_search_engine;
        g_search_engine = nullptr;
    }
}

/**
 * 主函数
 */
int main() {
    try {
        std::cout << "=== Boost Search Engine Starting ===" << std::endl;

        // 初始化搜索引擎
        if (!initialize_search_engine()) {
            return 1;
        }

        // 创建IO服务
        boost::asio::io_service io_service;

        // 启动HTTP服务器（端口9882）
        HttpServer server(io_service, 9882);

        std::cout << "HTTP server started, listening on port: 9882" << std::endl;
        std::cout << "Please visit: http://localhost:9882" << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;

        // 运行IO服务
        io_service.run();
    }
    catch (std::exception& e) {
        std::cerr << "Program exception: " << e.what() << std::endl;
    }

    // 清理资源
    cleanup();

    return 0;
}

/**
 * 获取全局搜索引擎实例
 */
SearchEngine* get_search_engine() {
    return g_search_engine;
}
