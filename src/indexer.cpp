/**
 * 索引构建器实现
 *
 * 功能：
 * 1. 扫描目录中的文件
 * 2. 解析不同类型的文档
 * 3. 提取文档内容和元数据
 */

#include "indexer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = boost::filesystem;

Indexer::Indexer() {
    // 初始化支持的文件扩展名
    supported_extensions_.push_back(".txt");
    supported_extensions_.push_back(".html");
    supported_extensions_.push_back(".htm");
    supported_extensions_.push_back(".md");
    supported_extensions_.push_back(".cpp");
    supported_extensions_.push_back(".h");
    supported_extensions_.push_back(".c");
    supported_extensions_.push_back(".hpp");

    std::cout << "索引构建器初始化完成" << std::endl;
}

Indexer::~Indexer() {
    std::cout << "索引构建器清理资源" << std::endl;
}

std::vector<Document> Indexer::scan_directory(const std::string& directory_path) {
    std::vector<Document> documents;

    try {
        if (!fs::exists(directory_path)) {
            std::cout << "Directory does not exist: " << directory_path << std::endl;
            return documents;
        }

        if (!fs::is_directory(directory_path)) {
            std::cout << "Path is not a directory: " << directory_path << std::endl;
            return documents;
        }

        std::cout << "Scanning directory: " << directory_path << std::endl;

        // 递归遍历目录
        fs::recursive_directory_iterator end_iter;
        for (fs::recursive_directory_iterator iter(directory_path); iter != end_iter; ++iter) {
            try {
                if (fs::is_regular_file(iter->status())) {
                    std::string file_path = iter->path().string();

                    if (is_supported_file(file_path)) {
                        std::cout << "Processing file: " << file_path << std::endl;
                        Document doc = parse_file(file_path);
                        if (!doc.content.empty()) {
                            documents.push_back(doc);
                        }
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing file: " << e.what() << std::endl;
                continue;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }

    std::cout << "Scan completed, found " << documents.size() << " documents" << std::endl;
    return documents;
}

Document Indexer::parse_file(const std::string& file_path) {
    std::string doc_id = generate_doc_id(file_path);
    std::string title = extract_title(file_path);
    std::string content;

    try {
        fs::path path(file_path);
        std::string extension = path.extension().string();
        boost::to_lower(extension);

        if (extension == ".html" || extension == ".htm") {
            content = parse_html_file(file_path);
        } else {
            content = parse_text_file(file_path);
        }

        // 限制内容长度，避免过大的文件
        if (content.length() > 10000) {
            content = content.substr(0, 10000) + "...";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse file " << file_path << ": " << e.what() << std::endl;
    }

    return Document(doc_id, title, content, file_path);
}

bool Indexer::is_supported_file(const std::string& file_path) {
    fs::path path(file_path);
    std::string extension = path.extension().string();
    boost::to_lower(extension);

    for (const std::string& supported_ext : supported_extensions_) {
        if (extension == supported_ext) {
            return true;
        }
    }

    return false;
}

std::string Indexer::parse_text_file(const std::string& file_path) {
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    // 读取文件内容
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // 使用编码检测和转换功能
    content = detect_and_convert_encoding(content);

    return content;
}

std::string Indexer::parse_html_file(const std::string& file_path) {
    std::string html_content = parse_text_file(file_path);

    // 简单的HTML标签移除（使用正则表达式）
    boost::regex html_tag_regex("<[^>]*>");
    std::string text_content = boost::regex_replace(html_content, html_tag_regex, " ");

    // 解码HTML实体（简单版本）
    boost::replace_all(text_content, "&nbsp;", " ");
    boost::replace_all(text_content, "&lt;", "<");
    boost::replace_all(text_content, "&gt;", ">");
    boost::replace_all(text_content, "&amp;", "&");
    boost::replace_all(text_content, "&quot;", "\"");

    // 清理多余的空白字符
    boost::regex whitespace_regex("\\s+");
    text_content = boost::regex_replace(text_content, whitespace_regex, " ");
    boost::trim(text_content);

    return text_content;
}

std::string Indexer::generate_doc_id(const std::string& file_path) {
    // 使用文件路径的哈希值作为文档ID
    std::hash<std::string> hasher;
    size_t hash_value = hasher(file_path);

    std::ostringstream oss;
    oss << "doc_" << hash_value;
    return oss.str();
}

std::string Indexer::extract_title(const std::string& file_path) {
    fs::path path(file_path);
    std::string filename = path.filename().string();

    // 对文件名进行编码检测和转换
    filename = detect_and_convert_encoding(filename);

    // 移除文件扩展名
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        filename = filename.substr(0, dot_pos);
    }

    // 替换下划线和连字符为空格
    boost::replace_all(filename, "_", " ");
    boost::replace_all(filename, "-", " ");

    return filename;
}

// 编码检测和转换函数
std::string Indexer::detect_and_convert_encoding(const std::string& raw_content) {
    try {
        // 移除BOM标记
        std::string content = remove_bom(raw_content);

        // 检测编码
        std::string encoding = detect_encoding(content);

        //std::cout << "检测到文件编码: " << encoding << std::endl; // 输出检测文件编码的结果

        // 根据检测结果进行转换
        if (encoding == "GBK" || encoding == "GB2312") {
            return convert_gbk_to_utf8(content); // GBK -> UTF-8 转换
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

std::string Indexer::detect_encoding(const std::string& content) {
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

std::string Indexer::remove_bom(const std::string& content) {
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

std::string Indexer::convert_gbk_to_utf8(const std::string& gbk_content) {
#ifdef _WIN32
    try {
        // 使用Windows API进行编码转换

        // 第一步：GBK -> Unicode (UTF-16)
        int unicode_len = MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, NULL, 0);
        if (unicode_len <= 0) {
            std::cerr << "GBK到Unicode转换失败" << std::endl;
            return gbk_content;
        }

        std::vector<wchar_t> unicode_str(unicode_len);
        MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, &unicode_str[0], unicode_len);

        // 第二步：Unicode (UTF-16) -> UTF-8
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, &unicode_str[0], -1, NULL, 0, NULL, NULL);
        if (utf8_len <= 0) {
            std::cerr << "Unicode到UTF-8转换失败" << std::endl;
            return gbk_content;
        }

        std::vector<char> utf8_str(utf8_len);
        WideCharToMultiByte(CP_UTF8, 0, &unicode_str[0], -1, &utf8_str[0], utf8_len, NULL, NULL);

        return std::string(&utf8_str[0]);

    } catch (const std::exception& e) {
        std::cerr << "Windows API编码转换异常: " << e.what() << std::endl;
        return gbk_content;
    }
#else
    // 非Windows平台直接返回原内容
    return gbk_content;
#endif
}
