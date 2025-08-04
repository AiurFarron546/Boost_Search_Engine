/**
 * @file indexer.cpp
 * @brief 索引构建器的实现文件
 *
 * 该文件负责实现`Indexer`类，用于扫描指定目录，
 * 解析支持的文件类型，并提取文档信息用于后续的索引和搜索。
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

// 使用boost::filesystem的命名空间简化代码
namespace fs = boost::filesystem;

/**
 * @brief Indexer类的构造函数
 *
 * 初始化支持的文件扩展名列表。
 */
Indexer::Indexer() {
    // 添加支持的文本和代码文件扩展名
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

/**
 * @brief Indexer类的析构函数
 *
 * 用于清理资源，当前为空。
 */
Indexer::~Indexer() {
    std::cout << "索引构建器清理资源" << std::endl;
}

/**
 * @brief 扫描指定目录并解析所有支持的文件
 * @param directory_path 要扫描的目录路径
 * @return 包含已解析文档信息的向量
 */
std::vector<Document> Indexer::scan_directory(const std::string& directory_path) {
    std::vector<Document> documents;

    try {
        // 检查目录是否存在且是否为目录
        if (!fs::exists(directory_path)) {
            std::cout << "Directory does not exist: " << directory_path << std::endl;
            return documents;
        }
        if (!fs::is_directory(directory_path)) {
            std::cout << "Path is not a directory: " << directory_path << std::endl;
            return documents;
        }

        std::cout << "Scanning directory: " << directory_path << std::endl;

        // 使用递归迭代器遍历目录及其所有子目录
        fs::recursive_directory_iterator end_iter;
        for (fs::recursive_directory_iterator iter(directory_path); iter != end_iter; ++iter) {
            try {
                // 只处理普通文件
                if (fs::is_regular_file(iter->status())) {
                    std::string file_path = iter->path().string();

                    // 检查文件扩展名是否受支持
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
                std::cerr << "Error processing file: " << iter->path().string() << " - " << e.what() << std::endl;
                continue; // 继续处理下一个文件
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << directory_path << " - " << e.what() << std::endl;
    }

    std::cout << "Scan completed, found " << documents.size() << " documents" << std::endl;
    return documents;
}

/**
 * @brief 解析单个文件，提取信息并创建Document对象
 * @param file_path 文件的完整路径
 * @return 解析后的Document对象
 */
Document Indexer::parse_file(const std::string& file_path) {
    std::string doc_id = generate_doc_id(file_path);
    std::string title = extract_title(file_path);
    std::string content;

    try {
        fs::path path(file_path);
        std::string extension = path.extension().string();
        boost::to_lower(extension); // 转换为小写以进行比较

        // 根据文件类型选择不同的解析方法
        if (extension == ".html" || extension == ".htm") {
            content = parse_html_file(file_path);
        } else {
            content = parse_text_file(file_path);
        }

        // 限制内容长度，避免索引过大的文件，提高性能
        if (content.length() > 10000) {
            content = content.substr(0, 10000) + "...";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to parse file " << file_path << ": " << e.what() << std::endl;
    }

    return Document(doc_id, title, content, file_path);
}

/**
 * @brief 检查文件扩展名是否在支持列表中
 * @param file_path 文件的路径
 * @return 如果支持则返回true，否则返回false
 */
bool Indexer::is_supported_file(const std::string& file_path) {
    fs::path path(file_path);
    std::string extension = path.extension().string();
    boost::to_lower(extension); // 转换为小写

    for (const std::string& supported_ext : supported_extensions_) {
        if (extension == supported_ext) {
            return true;
        }
    }

    return false;
}

/**
 * @brief 解析纯文本文件，读取其内容并进行编码转换
 * @param file_path 文件路径
 * @return 文件的UTF-8编码内容
 */
std::string Indexer::parse_text_file(const std::string& file_path) {
    std::ifstream file(file_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    // 将文件内容读入字符串
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    // 自动检测编码并转换为UTF-8
    return detect_and_convert_encoding(content);
}

/**
 * @brief 解析HTML文件，提取纯文本内容
 * @param file_path HTML文件的路径
 * @return 提取出的纯文本内容
 */
std::string Indexer::parse_html_file(const std::string& file_path) {
    // 首先像处理文本文件一样读取并转换编码
    std::string html_content = parse_text_file(file_path);

    // 使用正则表达式移除所有HTML标签
    boost::regex html_tag_regex("<[^>]*>");
    std::string text_content = boost::regex_replace(html_content, html_tag_regex, " ");

    // 解码常见的HTML实体
    boost::replace_all(text_content, "&nbsp;", " ");
    boost::replace_all(text_content, "<", "<");
    boost::replace_all(text_content, ">", ">");
    boost::replace_all(text_content, "&", "&");
    boost::replace_all(text_content, """, "\"");

    // 使用正则表达式将多个连续的空白字符替换为单个空格
    boost::regex whitespace_regex("\\s+");
    text_content = boost::regex_replace(text_content, whitespace_regex, " ");
    boost::trim(text_content); // 移除首尾的空白

    return text_content;
}

/**
 * @brief 根据文件路径生成一个唯一的文档ID
 * @param file_path 文件路径
 * @return 生成的文档ID字符串
 */
std::string Indexer::generate_doc_id(const std::string& file_path) {
    // 使用标准库的hash函数为文件路径生成一个哈希值
    std::hash<std::string> hasher;
    size_t hash_value = hasher(file_path);

    // 将哈希值转换为字符串ID
    std::ostringstream oss;
    oss << "doc_" << hash_value;
    return oss.str();
}

/**
 * @brief 从文件路径中提取一个合适的标题
 * @param file_path 文件路径
 * @return 提取并处理后的文件名作为标题
 */
std::string Indexer::extract_title(const std::string& file_path) {
    fs::path path(file_path);
    std::string filename = path.filename().string();

    // 文件名本身可能也需要编码转换
    filename = detect_and_convert_encoding(filename);

    // 移除文件扩展名
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos) {
        filename = filename.substr(0, dot_pos);
    }

    // 将下划线和连字符替换为空格，使标题更易读
    boost::replace_all(filename, "_", " ");
    boost::replace_all(filename, "-", " ");

    return filename;
}

/**
 * @brief 检测文件内容的编码并将其转换为UTF-8
 * @param raw_content 原始文件内容
 * @return 转换为UTF-8后的文件内容
 */
std::string Indexer::detect_and_convert_encoding(const std::string& raw_content) {
    try {
        std::string content = remove_bom(raw_content);
        std::string encoding = detect_encoding(content);

        if (encoding == "GBK" || encoding == "GB2312") {
            return convert_gbk_to_utf8(content);
        } else {
            return content;
        }
    } catch (const std::exception& e) {
        std::cerr << "编码处理异常: " << e.what() << std::endl;
        return raw_content;
    }
}

/**
 * @brief 简单地检测字符串的编码格式
 * @param content 待检测的字符串内容
 * @return 检测到的编码格式字符串 ("UTF-8" 或 "GBK")
 */
std::string Indexer::detect_encoding(const std::string& content) {
    if (content.empty()) return "UTF-8";

    if (content.length() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return "UTF-8";
    }

    size_t utf8_chars = 0;
    size_t high_ascii = 0;
    for (size_t i = 0; i < content.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(content[i]);
        if (c > 127) {
            high_ascii++;
            if ((c & 0xE0) == 0xC0 && i + 1 < content.length() && (static_cast<unsigned char>(content[i + 1]) & 0xC0) == 0x80) {
                utf8_chars++;
                i++;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < content.length() && (static_cast<unsigned char>(content[i + 1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(content[i + 2]) & 0xC0) == 0x80) {
                utf8_chars++;
                i += 2;
            }
        }
    }

    if (high_ascii > 0 && utf8_chars * 2 >= high_ascii) {
        return "UTF-8";
    } else if (high_ascii > 0) {
        return "GBK";
    }

    return "UTF-8";
}

/**
 * @brief 从字符串内容中移除BOM (Byte Order Mark)
 * @param content 包含BOM的字符串
 * @return 移除BOM后的字符串
 */
std::string Indexer::remove_bom(const std::string& content) {
    if (content.length() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        return content.substr(3);
    }
    if (content.length() >= 2 &&
        ((static_cast<unsigned char>(content[0]) == 0xFF && static_cast<unsigned char>(content[1]) == 0xFE) ||
         (static_cast<unsigned char>(content[0]) == 0xFE && static_cast<unsigned char>(content[1]) == 0xFF))) {
        return content.substr(2);
    }
    return content;
}

/**
 * @brief 将GBK编码的字符串转换为UTF-8编码
 * @param gbk_content GBK编码的字符串
 * @return UTF-8编码的字符串
 */
std::string Indexer::convert_gbk_to_utf8(const std::string& gbk_content) {
#ifdef _WIN32
    // 在Windows平台下，使用Windows API进行精确的编码转换
    try {
        int unicode_len = MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, NULL, 0);
        if (unicode_len <= 0) {
            std::cerr << "GBK到Unicode转换失败" << std::endl;
            return gbk_content;
        }
        std::vector<wchar_t> unicode_str(unicode_len);
        MultiByteToWideChar(CP_ACP, 0, gbk_content.c_str(), -1, &unicode_str[0], unicode_len);

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
    // 在非Windows平台，目前实现为直接返回原内容。
    // 生产环境建议使用iconv等库进行转换。
    return gbk_content;
#endif
}
