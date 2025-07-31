#ifndef SEARCH_ENGINE_H
#define SEARCH_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <boost/thread/shared_mutex.hpp>

/**
 * 搜索结果结构体
 */
struct SearchResult {
    std::string title;      // 文档标题
    std::string content;    // 文档内容摘要
    std::string url;        // 文档URL或路径
    double score;           // 相关性分数

    SearchResult(const std::string& t, const std::string& c, const std::string& u, double s)
        : title(t), content(c), url(u), score(s) {}
};

/**
 * 搜索引擎核心类
 */
class SearchEngine
{
public:
    SearchEngine();
    ~SearchEngine();

    // 添加文档到索引
    void add_document(const std::string& doc_id, const std::string& title, const std::string& content);

    // 执行搜索
    std::vector<SearchResult> search(const std::string& query, int max_results = 10);

    // 构建索引
    void build_index();

    // 加载数据文件
    void load_data_files(const std::string& data_dir);

    // 获取文档内容
    std::pair<std::string, std::string> get_document(const std::string& doc_id);

private:
    // 倒排索引：词项 -> 文档ID集合
    std::map<std::string, std::set<std::string>> inverted_index_;

    // 文档存储：文档ID -> 文档信息
    std::map<std::string, std::pair<std::string, std::string>> documents_;

    // 词频统计：文档ID -> (词项 -> 频率)
    std::map<std::string, std::map<std::string, int>> term_frequency_;

    // 文档频率：词项 -> 包含该词项的文档数量
    std::map<std::string, int> document_frequency_;

    // 读写锁，支持并发读取
    mutable boost::shared_mutex mutex_;

    // 计算TF-IDF分数
    double calculate_tfidf(const std::string& term, const std::string& doc_id);

    // 计算文档相关性分数
    double calculate_relevance_score(const std::string& doc_id, const std::vector<std::string>& query_terms);
};

#endif // SEARCH_ENGINE_H
