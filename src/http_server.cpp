/**
 * HTTP服务器实现
 *
 * 功能：
 * 1. 处理HTTP请求
 * 2. 提供静态文件服务
 * 3. 处理搜索API请求
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

// 外部函数声明
extern SearchEngine* get_search_engine();

/**
 * HttpConnection实现
 */
HttpConnection::pointer HttpConnection::create(boost::asio::io_service& io_service) {
    return pointer(new HttpConnection(io_service));
}

HttpConnection::HttpConnection(boost::asio::io_service& io_service)
    : socket_(io_service) {
}

tcp::socket& HttpConnection::socket() {
    return socket_;
}

void HttpConnection::start() {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&HttpConnection::handle_read, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void HttpConnection::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
        std::string request(data_, bytes_transferred);
        std::string response = process_request(request);

        boost::asio::async_write(socket_, boost::asio::buffer(response),
            boost::bind(&HttpConnection::handle_write, shared_from_this(),
                boost::asio::placeholders::error));
    }
}

void HttpConnection::handle_write(const boost::system::error_code& error) {
    if (!error) {
        // 连接处理完成，可以关闭或保持连接
    }
}

std::string HttpConnection::process_request(const std::string& request) {
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;

    std::cout << "收到请求: " << method << " " << path << std::endl;

    // 处理根路径
    if (path == "/") {
        path = "/index.html";
    }

    // 处理搜索API
    if (path.find("/api/search") == 0) {
        // 解析查询参数
        size_t query_pos = path.find("?q=");
        if (query_pos != std::string::npos) {
            std::string query = path.substr(query_pos + 3);

            // URL解码
            query = url_decode(query);

            std::cout << "Decoded query: " << query << std::endl;

            // 执行搜索
            SearchEngine* engine = get_search_engine();
            if (engine) {
                auto results = engine->search(query, 10);

                // 构建JSON响应
                std::ostringstream json;
                json << "{\"results\":[";
                for (size_t i = 0; i < results.size(); ++i) {
                    if (i > 0) json << ",";

                    // 构建文档查看URL
                    std::string doc_url = "/doc/" + results[i].url;

                    json << "{"
                         << "\"title\":\"" << escape_json(results[i].title) << "\","
                         << "\"content\":\"" << escape_json(results[i].content) << "\","
                         << "\"url\":\"" << escape_json(doc_url) << "\","
                         << "\"score\":" << results[i].score
                         << "}";
                }
                json << "],\"total\":" << results.size() << "}";

                //std::cout << "JSON response: " << json.str() << std::endl;//后端输出json数据，便于调试
                return create_response(json.str(), "application/json");
            }
        }
        return create_response("{\"error\":\"Invalid query\",\"total\":0}", "application/json");
    }

    // 处理文档查看请求
    if (path.find("/doc/") == 0) {
        std::string doc_id = path.substr(5); // 移除 "/doc/" 前缀
        return serve_document(doc_id);
    }

    // 处理静态文件
    std::string file_path = "web" + path;
    std::string content = get_file_content(file_path);

    if (!content.empty()) {
        std::string content_type = "text/html";
        if (path.find(".css") != std::string::npos) {
            content_type = "text/css";
        } else if (path.find(".js") != std::string::npos) {
            content_type = "application/javascript";
        }
        return create_response(content, content_type);
    }

    // 404错误
    return create_response("<h1>404 Not Found</h1>", "text/html");
}

std::string HttpConnection::create_response(const std::string& content, const std::string& content_type) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << content_type << "; charset=utf-8\r\n"
             << "Content-Length: " << content.length() << "\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << content;
    return response.str();
}

std::string HttpConnection::get_file_content(const std::string& file_path) {
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    // 读取原始内容
    std::ostringstream raw_content;
    raw_content << file.rdbuf();
    std::string content = raw_content.str();

    // 检测并转换编码
    return detect_and_convert_encoding(content);
}

std::string HttpConnection::url_decode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.length());

    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // 解码十六进制字符
            std::string hex = encoded.substr(i + 1, 2);
            char ch = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            decoded += ch;
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }

    return decoded;
}

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
                // 只转义ASCII控制字符，保持UTF-8字符完整性
                if (static_cast<unsigned char>(c) < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<unsigned char>(c);
                    escaped += oss.str();
                } else {
                    // 直接保留字符，包括UTF-8多字节字符
                    escaped += c;
                }
                break;
        }
    }

    return escaped;
}

std::string HttpConnection::serve_document(const std::string& doc_id) {
    SearchEngine* engine = get_search_engine();
    if (!engine) {
        return create_response("<h1>服务器错误</h1><p>搜索引擎未初始化</p>", "text/html");
    }

    // 获取文档内容
    auto doc_info = engine->get_document(doc_id);
    if (doc_info.first.empty()) {
        return create_response("<h1>404 Not Found</h1><p>文档不存在</p>", "text/html");
    }

    // 构建HTML页面
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

// 编码检测和转换函数实现
std::string HttpConnection::detect_and_convert_encoding(const std::string& raw_content) {
    try {
        // 移除BOM标记
        std::string content = remove_bom(raw_content);

        // 检测编码
        std::string encoding = detect_encoding(content);

        std::cout << "检测到文件编码: " << encoding << std::endl;

        // 根据检测结果进行转换
        if (encoding == "GBK" || encoding == "GB2312") {
            return convert_gbk_to_utf8(content);
        } else if (encoding == "UTF-8") {
            return content; // 已经是UTF-8，直接返回
        } else {
            // 未知编码，尝试作为UTF-8处理
            return content;
        }
    } catch (const std::exception& e) {
        std::cerr << "编码处理异常: " << e.what() << std::endl;
        return raw_content; // 异常情况下返回原始内容
    }
}

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

    // 简单的编码检测逻辑
    size_t utf8_chars = 0;
    size_t total_chars = 0;
    size_t high_ascii = 0;

    for (size_t i = 0; i < content.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(content[i]);

        if (c > 127) {
            high_ascii++;

            // 检查是否为有效的UTF-8序列
            if ((c & 0xE0) == 0xC0 && i + 1 < content.length()) {
                // 2字节UTF-8
                unsigned char c2 = static_cast<unsigned char>(content[i + 1]);
                if ((c2 & 0xC0) == 0x80) {
                    utf8_chars++;
                    i++; // 跳过下一个字节
                }
            } else if ((c & 0xF0) == 0xE0 && i + 2 < content.length()) {
                // 3字节UTF-8
                unsigned char c2 = static_cast<unsigned char>(content[i + 1]);
                unsigned char c3 = static_cast<unsigned char>(content[i + 2]);
                if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
                    utf8_chars++;
                    i += 2; // 跳过接下来的两个字节
                }
            }
        }
        total_chars++;
    }

    // 如果高位ASCII字符中大部分是有效UTF-8，则认为是UTF-8
    if (high_ascii > 0 && utf8_chars * 2 >= high_ascii) {
        return "UTF-8";
    } else if (high_ascii > 0) {
        return "GBK"; // 否则可能是GBK
    }

    return "UTF-8"; // 默认UTF-8
}

std::string HttpConnection::remove_bom(const std::string& content) {
    // 移除UTF-8 BOM
    if (content.length() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return content.substr(3);
    }

    // 移除UTF-16 BOM
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

std::string HttpConnection::convert_gbk_to_utf8(const std::string& gbk_content) {
#ifdef _WIN32
    try {
        // 使用Windows API进行编码转换

        // 第一步：GBK -> Unicode (UTF-16)
        int unicode_len = MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, NULL, 0);
        if (unicode_len <= 0) {
            std::cerr << "GBK到Unicode转换失败" << std::endl;
            return simple_gbk_to_utf8(gbk_content);
        }

        std::vector<wchar_t> unicode_str(unicode_len);
        MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, &unicode_str[0], unicode_len);

        // 第二步：Unicode (UTF-16) -> UTF-8
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &unicode_str[0], -1, NULL, 0, NULL, NULL);
        if (utf8_len <= 0) {
            std::cerr << "Unicode到UTF-8转换失败" << std::endl;
            return simple_gbk_to_utf8(gbk_content);
        }

        std::vector<char> utf8_str(utf8_len);
        WideCharToMultiByte(CP_UTF8, 0, &unicode_str[0], -1, &utf8_str[0], utf8_len, NULL, NULL);

        return std::string(&utf8_str[0]);

    } catch (const std::exception& e) {
        std::cerr << "Windows API编码转换异常: " << e.what() << std::endl;
        return simple_gbk_to_utf8(gbk_content);
    }
#else
    // 非Windows平台使用简单转换
    return simple_gbk_to_utf8(gbk_content);
#endif
}

std::string HttpConnection::simple_gbk_to_utf8(const std::string& gbk_content) {
    // 简单的GBK到UTF-8转换（仅处理常见字符）

    std::string result;
    result.reserve(gbk_content.length() * 2);

    for (size_t i = 0; i < gbk_content.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(gbk_content[i]);

        if (c < 0x80) {
            // ASCII字符直接复制
            result += static_cast<char>(c);
        } else {
            // 高位字符，可能是GBK编码
            // 这里只是简单处理，实际应该查表转换
            result += static_cast<char>(c);
        }
    }

    return result;
}

/**
 * HttpServer实现
 */
HttpServer::HttpServer(boost::asio::io_service& io_service, short port)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
}

void HttpServer::start_accept() {
    HttpConnection::pointer new_connection = HttpConnection::create(static_cast<boost::asio::io_service&>(acceptor_.get_executor().context()));

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&HttpServer::handle_accept, this, new_connection,
            boost::asio::placeholders::error));
}

void HttpServer::handle_accept(HttpConnection::pointer new_connection, const boost::system::error_code& error) {
    if (!error) {
        new_connection->start();
    }

    start_accept();
}
