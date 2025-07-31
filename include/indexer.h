#ifndef INDEXER_H
#define INDEXER_H

#include <string>
#include <vector>
#include <boost/filesystem.hpp>

/**
 * 文档信息结构体
 */
struct Document {
    std::string id;         // 文档唯一标识
    std::string title;      // 文档标题
    std::string content;    // 文档内容
    std::string file_path;  // 文件路径

    Document(const std::string& i, const std::string& t, const std::string& c, const std::string& p)
        : id(i), title(t), content(c), file_path(p) {}
};

/**
 * 索引构建器类
 */
class Indexer
{
public:
    Indexer();
    ~Indexer();

    // 扫描目录并构建文档列表
    std::vector<Document> scan_directory(const std::string& directory_path);

    // 解析单个文件
    Document parse_file(const std::string& file_path);

    // 支持的文件类型检查
    bool is_supported_file(const std::string& file_path);

private:
    // 支持的文件扩展名
    std::vector<std::string> supported_extensions_;

    // 解析文本文件
    std::string parse_text_file(const std::string& file_path);

    // 解析HTML文件
    std::string parse_html_file(const std::string& file_path);

    // 从文件路径生成文档ID
    std::string generate_doc_id(const std::string& file_path);

    // 从文件路径提取标题
    std::string extract_title(const std::string& file_path);

    // 编码检测和转换函数
    std::string detect_and_convert_encoding(const std::string& raw_content);
    std::string detect_encoding(const std::string& content);
    std::string remove_bom(const std::string& content);
    std::string convert_gbk_to_utf8(const std::string& gbk_content);
};

#endif // INDEXER_H
