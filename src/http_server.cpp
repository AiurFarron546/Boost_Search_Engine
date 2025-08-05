/**
 * @file http_server.cpp
 * @brief HTTP服务器的实现文件
 *
 * 该文件包含了`HttpConnection`和`HttpServer`类的具体实现，
 * 用于处理HTTP连接、解析请求、提供静态文件和API服务。
 */

#include "http_server.h"
#include "search_engine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <boost/algorithm/string.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

// 外部函数声明，用于获取全局的搜索引擎实例
extern SearchEngine* get_search_engine();

/**
 * @brief HttpConnection类的实现，处理单个HTTP连接
 */

/**
 * @brief 创建一个新的HttpConnection实例的工厂方法
 * @param io_service Boost.Asio的io_service对象
 * @return 指向新创建的HttpConnection的共享指针
 */
HttpConnection::pointer HttpConnection::create(boost::asio::io_context& io_context) {
    return pointer(new HttpConnection(io_context));
}

/**
 * @brief HttpConnection的构造函数
 * @param io_service Boost.Asio的io_service对象
 */
HttpConnection::HttpConnection(boost::asio::io_context& io_context)
    : socket_(io_context) {
}

/**
 * @brief 获取当前连接的socket
 * @return TCP socket的引用
 */
tcp::socket& HttpConnection::socket() {
    return socket_;
}

/**
 * @brief 启动异步读取操作，开始处理连接
 */
void HttpConnection::start() {
    // 异步从socket读取数据到缓冲区data_
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&HttpConnection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

/**
 * @brief 异步读取完成后的回调函数
 * @param error 错误码
 * @param bytes_transferred 传输的字节数
 */
void HttpConnection::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
        // 将读取到的数据转换为字符串请求
        std::string request(data_, bytes_transferred);
        // 处理请求并生成响应
        std::string response = process_request(request);

        // 异步将响应写回socket
        boost::asio::async_write(socket_, boost::asio::buffer(response),
            boost::bind(&HttpConnection::handle_write, shared_from_this(),
                boost::asio::placeholders::error));
    }
}

/**
 * @brief 异步写入完成后的回调函数
 * @param error 错误码
 */
void HttpConnection::handle_write(const boost::system::error_code& error) {
    if (!error) {
        // 写入成功，连接处理完成。可以根据需要关闭或保持连接。
        // 当前实现是短连接，在写入后由客户端或服务器自动关闭。
    }
}

/**
 * @brief 解析并处理HTTP请求，生成响应字符串
 * @param request 原始HTTP请求字符串
 * @return HTTP响应字符串
 */
std::string HttpConnection::process_request(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version; // 解析请求行

    std::cout << "收到请求: " << method << " " << path << std::endl;

    // 如果请求根路径，则默认返回index.html
    if (path == "/") {
        path = "/index.html";
    }

    // 处理搜索API请求，路径以/api/search开头
    if (path.find("/api/search") == 0) {
        // 解析URL中的查询参数
        size_t query_pos = path.find("?q=");
        if (query_pos != std::string::npos) {
            std::string query = path.substr(query_pos + 3);

            // 对查询字符串进行URL解码
            query = url_decode(query);
            std::cout << "Decoded query: " << query << std::endl;

            // 获取搜索引擎实例并执行搜索
            SearchEngine* engine = get_search_engine();
            if (engine) {
                auto results = engine->search(query, 10); // 最多返回10条结果

                // 构建JSON格式的响应
                std::ostringstream json;
                json << "{\"results\":[";
                for (size_t i = 0; i < results.size(); ++i) {
                    if (i > 0) json << ",";
                    // 为每个文档构建一个可访问的URL
                    std::string doc_url = "/doc/" + results[i].url;
                    json << "{"
                         << "\"title\":\"" << escape_json(results[i].title) << "\","
                         << "\"content\":\"" << escape_json(results[i].content) << "\","
                         << "\"url\":\"" << escape_json(doc_url) << "\","
                         << "\"score\":" << results[i].score
                         << "}";
                }
                json << "],\"total\":" << results.size() << "}";

                return create_response(json.str(), "application/json");
            }
        }
        // 如果查询无效，返回错误信息
        return create_response("{\"error\":\"Invalid query\",\"total\":0}", "application/json");
    }

    // 处理文档查看请求，路径以/doc/开头
    if (path.find("/doc/") == 0) {
        std::string doc_id = path.substr(5); // 提取文档ID
        return serve_document(doc_id);
    }

    // 处理静态文件请求
    std::string file_path = "web" + path; // 静态文件存储在web目录下
    std::string content = get_file_content(file_path);

    if (!content.empty()) {
        // 根据文件扩展名确定Content-Type
        std::string content_type = "text/html";
        if (path.find(".css") != std::string::npos) {
            content_type = "text/css";
        } else if (path.find(".js") != std::string::npos) {
            content_type = "application/javascript";
        }
        return create_response(content, content_type);
    }

    // 如果文件未找到，返回404错误
    return create_response("<h1>404 Not Found</h1>", "text/html");
}

/**
 * @brief 构建一个完整的HTTP响应
 * @param content 响应体内容
 * @param content_type 响应内容的MIME类型
 * @return 完整的HTTP响应字符串
 */
std::string HttpConnection::create_response(const std::string& content, const std::string& content_type) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << content_type << "; charset=utf-8\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n" // 允许跨域请求
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Connection: close\r\n" // 短连接
             << "\r\n"
             << content;
    return response.str();
}

/**
 * @brief 读取指定路径的文件内容
 * @param file_path 文件路径
 * @return 文件内容字符串，如果文件不存在则返回空字符串
 */
std::string HttpConnection::get_file_content(const std::string& file_path) {
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    // 读取文件原始内容
    std::ostringstream raw_content;
    raw_content << file.rdbuf();
    std::string content = raw_content.str();

    // 自动检测文件编码并转换为UTF-8
    return detect_and_convert_encoding(content);
}

/**
 * @brief 对URL编码的字符串进行解码
 * @param encoded URL编码的字符串
 * @return 解码后的字符串
 */
std::string HttpConnection::url_decode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length());

    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // 解码十六进制字符，例如 %E4 -> 'ä'
            std::string hex = encoded.substr(i + 1, 2);
            char ch = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            decoded += ch;
            i += 2;
        } else if (encoded[i] == '+') {
            // 将'+'转换为空格
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }

    return decoded;
}

/**
 * @brief 转义字符串中的特殊字符以符合JSON格式
 * @param str 待转义的原始字符串
 * @return 转义后的JSON字符串
 */
std::string HttpConnection::escape_json(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);

    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];

        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                // 只转义ASCII控制字符，以保持UTF-8字符的完整性
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned char>(c);
                    escaped += oss.str();
                } else {
                    // 直接保留其他字符，包括UTF-8多字节字符
                    escaped += c;
                }
                break;
        }
    }

    return escaped;
}

/**
 * @brief 根据文档ID提供文档的HTML页面
 * @param doc_id 文档的唯一标识符
 * @return 包含文档内容的HTML响应字符串
 */
std::string HttpConnection::serve_document(const std::string& doc_id) {
    SearchEngine* engine = get_search_engine();
    if (!engine) {
        return create_response("<h1>服务器错误</h1><p>搜索引擎未初始化</p>", "text/html");
    }

    // 从搜索引擎获取文档的标题和内容
    auto doc_info = engine->get_document(doc_id);
    if (doc_info.first.empty()) {
        return create_response("<h1>404 Not Found</h1><p>文档不存在</p>", "text/html");
    }

    // 构建显示文档的HTML页面
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"zh-CN\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "    <title>" << escape_html(doc_info.first) << "</title>\n"
         << "    <style>\n"
         << "        body { font-family: 'Microsoft YaHei', Arial, sans-serif; line-height: 1.6; margin: 40px; background: #f5f5f5; }\n"
         << "        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n"
         << "        h1 { color: #333; border-bottom: 2px solid #007bff; padding-bottom: 10px; }\n"
         << "        .content { white-space: pre-wrap; color: #555; }\n"
         << "        .back-link { display: inline-block; margin-top: 20px; color: #007bff; text-decoration: none; }\n"
         << "        .back-link:hover { text-decoration: underline; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <div class=\"container\">\n"
         << "        <h1>" << escape_html(doc_info.first) << "</h1>\n"
         << "        <div class=\"content\">" << escape_html(doc_info.second) << "</div>\n"
         << "        <a href=\"/\" class=\"back-link\">← 返回搜索</a>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>";

    return create_response(html.str(), "text/html");
}

/**
 * @brief 转义字符串中的HTML特殊字符
 * @param str 待转义的原始字符串
 * @return 转义后的HTML字符串
 */
std::string HttpConnection::escape_html(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 2);

    for (char c : str) {
        switch (c) {
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '&': escaped += "&amp;"; break;
            case '"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += c; break;
        }
    }

    return escaped;
}

/**
 * @brief 检测文件内容的编码并将其转换为UTF-8
 * @param raw_content 原始文件内容
 * @return 转换为UTF-8后的文件内容
 */
std::string HttpConnection::detect_and_convert_encoding(const std::string& raw_content) {
    try {
        // 移除可能存在的BOM (Byte Order Mark)
        std::string content = remove_bom(raw_content);

        // 检测内容的编码格式
        std::string encoding = detect_encoding(content);
        std::cout << "检测到文件编码: " << encoding << std::endl;

        // 如果是GBK或GB2312，则转换为UTF-8
        if (encoding == "GBK" || encoding == "GB2312") {
            return convert_gbk_to_utf8(content);
        } else {
            // 如果是UTF-8或未知编码，直接返回
            return content;
        }
    } catch (const std::exception& e) {
        std::cerr << "编码处理异常: " << e.what() << std::endl;
        return raw_content; // 发生异常时返回原始内容
    }
}

/**
 * @brief 简单地检测字符串的编码格式
 * @param content 待检测的字符串内容
 * @return 检测到的编码格式字符串 ("UTF-8" 或 "GBK")
 */
std::string HttpConnection::detect_encoding(const std::string& content) {
    if (content.empty()) {
        return "UTF-8";
    }

    // 检查UTF-8 BOM
    if (content.length() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return "UTF-8";
    }

    // 基于字符特征的简单编码检测逻辑
    size_t utf8_chars = 0;
    size_t high_ascii = 0;

    for (size_t i = 0; i < content.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(content[i]);

        if (c > 127) { // 非ASCII字符
            high_ascii++;
            // 检查是否为有效的UTF-8多字节序列
            if ((c & 0xE0) == 0xC0 && i + 1 < content.length()) { // 2字节UTF-8
                if ((static_cast<unsigned char>(content[i + 1]) & 0xC0) == 0x80) {
                    utf8_chars++;
                    i++;
                }
            } else if ((c & 0xF0) == 0xE0 && i + 2 < content.length()) { // 3字节UTF-8
                if ((static_cast<unsigned char>(content[i + 1]) & 0xC0) == 0x80 &&
                    (static_cast<unsigned char>(content[i + 2]) & 0xC0) == 0x80) {
                    utf8_chars++;
                    i += 2;
                }
            }
        }
    }

    // 如果大部分非ASCII字符都是有效的UTF-8序列，则认为是UTF-8
    if (high_ascii > 0 && utf8_chars * 2 >= high_ascii) {
        return "UTF-8";
    } else if (high_ascii > 0) {
        return "GBK"; // 否则，可能是GBK
    }

    return "UTF-8"; // 默认返回UTF-8
}

/**
 * @brief 从字符串内容中移除BOM (Byte Order Mark)
 * @param content 包含BOM的字符串
 * @return 移除BOM后的字符串
 */
std::string HttpConnection::remove_bom(const std::string& content) {
    // 移除UTF-8 BOM (EF BB BF)
    if (content.length() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return content.substr(3);
    }

    // 移除UTF-16 BOM (FF FE or FE FF)
    if (content.length() >= 2) {
        if ((static_cast<unsigned char>(content[0]) == 0xFF &&
             static_cast<unsigned char>(content[1]) == 0xFE) ||
            (static_cast<unsigned char>(content[0]) == 0xFE &&
             static_cast<unsigned char>(content[1]) == 0xFF)) {
            return content.substr(2);
        }
    }

    return content;
}

/**
 * @brief 将GBK编码的字符串转换为UTF-8编码
 * @param gbk_content GBK编码的字符串
 * @return UTF-8编码的字符串
 */
std::string HttpConnection::convert_gbk_to_utf8(const std::string& gbk_content) {
#ifdef _WIN32
    // 在Windows平台下，使用Windows API进行精确的编码转换
    try {
        // 第一步：将GBK (CP_ACP) 转换为宽字符 (UTF-16)
        int unicode_len = MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, NULL, 0);
        if (unicode_len <= 0) {
            std::cerr << "GBK到Unicode转换失败" << std::endl;
            return simple_gbk_to_utf8(gbk_content); // 失败时回退到简单转换
        }
        std::vector<wchar_t> unicode_str(unicode_len);
        MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, &unicode_str[0], unicode_len);

        // 第二步：将宽字符 (UTF-16) 转换为UTF-8
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &unicode_str[0], -1, NULL, 0, NULL, NULL);
        if (utf8_len <= 0) {
            std::cerr << "Unicode到UTF-8转换失败" << std::endl;
            return simple_gbk_to_utf8(gbk_content); // 失败时回退到简单转换
        }
        std::vector<char> utf8_str(utf8_len);
        WideCharToMultiByte(CP_UTF8, 0, &unicode_str[0], -1, &utf8_str[0], utf8_len, NULL, NULL);

        return std::string(&utf8_str[0]);

    } catch (const std::exception& e) {
        std::cerr << "Windows API编码转换异常: " << e.what() << std::endl;
        return simple_gbk_to_utf8(gbk_content); // 异常时回退
    }
#else
    // 在非Windows平台，使用一个简化的转换逻辑
    return simple_gbk_to_utf8(gbk_content);
#endif
}

/**
 * @brief 一个简化的GBK到UTF-8转换实现（不完整，作为备用）
 * @param gbk_content GBK编码的字符串
 * @return 尝试转换后的字符串
 */
std::string HttpConnection::simple_gbk_to_utf8(const std::string& gbk_content) {
    // 这个函数只是一个占位符，实际的GBK到UTF-8转换需要复杂的码表。
    // 在非Windows环境下，建议使用iconv等库。
    std::string result;
    result.reserve(gbk_content.length() * 2);

    for (size_t i = 0; i < gbk_content.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(gbk_content[i]);
        if (c < 0x80) {
            result += static_cast<char>(c); // ASCII字符直接复制
        } else {
            // 简单地将非ASCII字符复制，这在很多情况下是不正确的
            result += static_cast<char>(c);
        }
    }
    return result;
}

/**
 * @brief HttpServer类的实现，用于监听端口并接受HTTP连接
 */

/**
 * @brief HttpServer的构造函数
 * @param io_service Boost.Asio的io_service对象
 * @param port 服务器监听的端口号
 */
HttpServer::HttpServer(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
    start_accept(); // 开始接受连接
}

/**
 * @brief 开始一个异步接受操作，等待新的客户端连接
 */
void HttpServer::start_accept() {
    // 创建一个新的HttpConnection对象来处理下一个连接
    HttpConnection::pointer new_connection = HttpConnection::create(static_cast<boost::asio::io_context&>(acceptor_.get_executor().context()));

    // 异步等待连接
    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&HttpServer::handle_accept, this, new_connection,
            boost::asio::placeholders::error));
}

/**
 * @brief 异步接受完成后的回调函数
 * @param new_connection 指向新建立的连接的指针
 * @param error 错误码
 */
void HttpServer::handle_accept(HttpConnection::pointer new_connection, const boost::system::error_code& error) {
    if (!error) {
        // 如果没有错误，启动新连接的处理流程
        new_connection->start();
    }

    // 继续等待下一个连接
    start_accept();
}
