/**
 * @file search_engine.cpp
 * @brief 搜索引擎核心功能的实现文件
 *
 * 该文件实现了`SearchEngine`类，包括构建倒排索引、执行搜索、
 * 计算TF-IDF相关性分数以及返回排序后的搜索结果等核心功能。
 */

#include "search_engine.h"
#include "indexer.h"
#include "text_processor.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <boost/thread/locks.hpp>

/**
 * @brief SearchEngine类的构造函数
 */
SearchEngine::SearchEngine() {
    std::cout << "Search engine initializing..." << std::endl;
}

/**
 * @brief SearchEngine类的析构函数
 */
SearchEngine::~SearchEngine() {
    std::cout << "Search engine cleaning up resources..." << std::endl;
}

/**
 * @brief 向搜索引擎中添加一个文档
 * @param doc_id 文档的唯一ID
 * @param title 文档的标题
 * @param content 文档的内容
 *
 * 此函数会处理文本、更新倒排索引和词频信息。
 */
void SearchEngine::add_document(const std::string& doc_id, const std::string& title, const std::string& content) {
    // 使用写锁保护，因为要修改共享数据
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    // 1. 存储原始文档信息
    documents_[doc_id] = std::make_pair(title, content);

    // 2. 文本预处理
    TextProcessor processor;
    std::string processed_text = processor.preprocess_text(title + " " + content);
    std::vector<std::string> tokens = processor.tokenize(processed_text);
    tokens = processor.remove_stop_words(tokens);

    // 3. 更新索引和词频统计
    std::map<std::string, int> term_freq;
    for (const std::string& term : tokens) {
        term_freq[term]++;
        inverted_index_[term].insert(doc_id); // 更新倒排索引
    }

    // 存储该文档的词频信息
    term_frequency_[doc_id] = term_freq;

    // 更新每个词的文档频率
    for (const auto& pair : term_freq) {
        document_frequency_[pair.first]++;
    }

    std::cout << "Added document: " << doc_id << " (terms: " << term_freq.size() << ")" << std::endl;
}

/**
 * @brief 执行搜索查询
 * @param query用户的查询字符串
 * @param max_results 最大返回结果数
 * @return 排序后的搜索结果列表
 */
std::vector<SearchResult> SearchEngine::search(const std::string& query, int max_results) {
    // 使用读锁保护，因为只读取共享数据
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    std::cout << "Executing search: \"" << query << "\"" << std::endl;

    // 1. 预处理查询字符串
    TextProcessor processor;
    std::string processed_query = processor.preprocess_text(query);
    std::vector<std::string> query_terms = processor.tokenize(processed_query);
    query_terms = processor.remove_stop_words(query_terms);

    if (query_terms.empty()) {
        return std::vector<SearchResult>();
    }

    // 2. 查找包含查询词的候选文档
    std::set<std::string> candidate_docs;
    for (const std::string& term : query_terms) {
        auto it = inverted_index_.find(term);
        if (it != inverted_index_.end()) {
            if (candidate_docs.empty()) {
                candidate_docs = it->second;
            } else {
                // 当前实现为OR查询，合并所有包含任一查询词的文档
                std::set<std::string> temp;
                std::set_union(candidate_docs.begin(), candidate_docs.end(),
                              it->second.begin(), it->second.end(),
                              std::inserter(temp, temp.begin()));
                candidate_docs = temp;
            }
        }
    }

    // 3. 为每个候选文档计算相关性分数
    std::vector<std::pair<std::string, double>> scored_docs;
    for (const std::string& doc_id : candidate_docs) {
        double score = calculate_relevance_score(doc_id, query_terms);
        if (score > 0) {
            scored_docs.push_back(std::make_pair(doc_id, score));
        }
    }

    // 4. 按分数降序排序
    std::sort(scored_docs.begin(), scored_docs.end(),
              [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
                  return a.second > b.second;
              });

    // 5. 构建并返回最终的搜索结果
    std::vector<SearchResult> results;
    int count = 0;
    for (const auto& pair : scored_docs) {
        if (count >= max_results) break;

        const std::string& doc_id = pair.first;
        double score = pair.second;

        auto doc_it = documents_.find(doc_id);
        if (doc_it != documents_.end()) {
            const std::string& title = doc_it->second.first;
            std::string content = doc_it->second.second;

            // 生成内容摘要，并确保不截断UTF-8字符
            if (content.length() > 180) {
                size_t cut_pos = 180;
                while (cut_pos > 0 && (content[cut_pos] & 0x80) && !(content[cut_pos] & 0x40)) {
                    cut_pos--;
                }
                content = content.substr(0, cut_pos) + "...";
            }

            results.push_back(SearchResult(title, content, doc_id, score));
            count++;
        }
    }

    std::cout << "Search completed, found " << results.size() << " results" << std::endl;
    return results;
}

/**
 * @brief 构建索引（目前主要是一个状态报告函数）
 *
 * 实际的索引构建过程在`add_document`中动态进行。
 * 此函数可以用于触发批量优化或报告索引状态。
 */
void SearchEngine::build_index() {
    std::cout << "Starting to build index..." << std::endl;
    // 索引构建是动态的，在add_document中完成
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    std::cout << "Index build completed:" << std::endl;
    std::cout << "  Document count: " << documents_.size() << std::endl;
    std::cout << "  Vocabulary size: " << inverted_index_.size() << std::endl;
}

/**
 * @brief 从指定目录加载数据文件并建立索引
 * @param data_dir 包含数据文件的目录路径
 */
void SearchEngine::load_data_files(const std::string& data_dir) {
    std::cout << "Loading data files from directory: " << data_dir << std::endl;

    Indexer indexer;
    std::vector<Document> documents = indexer.scan_directory(data_dir);
    std::cout << "Found " << documents.size() << " documents" << std::endl;

    for (const Document& doc : documents) {
        add_document(doc.id, doc.title, doc.content);
    }

    // 如果目录为空，则添加一些示例数据以供演示
    if (documents.empty()) {
        std::cout << "No data files found, adding sample data..." << std::endl;
        add_document("doc1", "C++编程入门", "C++是一种通用的编程语言...");
        add_document("doc2", "Boost库详细介绍", "Boost库是为C++语言标准库提供扩展...");
        add_document("doc3", "搜索引擎原理", "搜索引擎的核心是倒排索引...");
        add_document("doc4", "网络编程基础", "网络编程涉及套接字编程...");
        add_document("doc5", "多线程编程", "多线程编程可以提高程序的并发性能...");
    }
}

/**
 * @brief 计算一个词在一个文档中的TF-IDF值
 * @param term 要计算的词
 * @param doc_id 文档ID
 * @return 计算出的TF-IDF分数
 */
double SearchEngine::calculate_tfidf(const std::string& term, const std::string& doc_id) {
    // 计算TF (Term Frequency)
    auto tf_it = term_frequency_.find(doc_id);
    if (tf_it == term_frequency_.end()) return 0.0;
    auto term_it = tf_it->second.find(term);
    if (term_it == tf_it->second.end()) return 0.0;
    double tf = static_cast<double>(term_it->second);
    int total_terms = 0;
    for (const auto& pair : tf_it->second) {
        total_terms += pair.second;
    }
    tf = total_terms > 0 ? tf / total_terms : 0.0; // 归一化TF

    // 计算IDF (Inverse Document Frequency)
    auto df_it = document_frequency_.find(term);
    if (df_it == document_frequency_.end()) return 0.0;
    double df = static_cast<double>(df_it->second);
    double total_docs = static_cast<double>(documents_.size());
    double idf = total_docs > df ? std::log(total_docs / df) : 0.0;

    return tf * idf;
}

/**
 * @brief 计算一个文档相对于一个查询的相关性分数
 * @param doc_id 文档ID
 * @param query_terms 查询分词后的词列表
 * @return 相关性总分
 */
double SearchEngine::calculate_relevance_score(const std::string& doc_id, const std::vector<std::string>& query_terms) {
    double score = 0.0;
    // 简单地将查询中每个词的TF-IDF值相加
    for (const std::string& term : query_terms) {
        score += calculate_tfidf(term, doc_id);
    }
    return score;
}

/**
 * @brief 根据文档ID获取文档的标题和内容
 * @param doc_id 文档ID
 * @return 一个包含标题和内容的pair，如果未找到则两者都为空
 */
std::pair<std::string, std::string> SearchEngine::get_document(const std::string& doc_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    auto it = documents_.find(doc_id);
    if (it != documents_.end()) {
        return it->second; // 返回 (title, content)
    }
    return std::make_pair("", ""); // 文档不存在
}
