/**
 * 搜索引擎核心实现
 *
 * 功能：
 * 1. 构建倒排索引
 * 2. 执行搜索查询
 * 3. 计算TF-IDF相关性分数
 * 4. 返回排序后的搜索结果
 */

#include "search_engine.h"
#include "indexer.h"
#include "text_processor.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <boost/thread/locks.hpp>

SearchEngine::SearchEngine() {
    std::cout << "Search engine initializing..." << std::endl;
}

SearchEngine::~SearchEngine() {
    std::cout << "Search engine cleaning up resources..." << std::endl;
}

void SearchEngine::add_document(const std::string& doc_id, const std::string& title, const std::string& content) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    // 存储文档信息
    documents_[doc_id] = std::make_pair(title, content);

    // 文本处理
    TextProcessor processor;
    std::string processed_text = processor.preprocess_text(title + " " + content);
    std::vector<std::string> tokens = processor.tokenize(processed_text);
    tokens = processor.remove_stop_words(tokens);

    // 统计词频
    std::map<std::string, int> term_freq;
    for (const std::string& term : tokens) {
        term_freq[term]++;
        inverted_index_[term].insert(doc_id);
    }

    // 存储文档的词频信息
    term_frequency_[doc_id] = term_freq;

    // 更新文档频率
    for (const auto& pair : term_freq) {
        document_frequency_[pair.first]++;
    }

    std::cout << "Added document: " << doc_id << " (terms: " << term_freq.size() << ")" << std::endl;
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int max_results) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    std::cout << "Executing search: \"" << query << "\"" << std::endl;

    // 处理查询文本
    TextProcessor processor;
    std::string processed_query = processor.preprocess_text(query);
    std::vector<std::string> query_terms = processor.tokenize(processed_query);
    query_terms = processor.remove_stop_words(query_terms);

    if (query_terms.empty()) {
        return std::vector<SearchResult>();
    }

    // 找到包含查询词的文档
    std::set<std::string> candidate_docs;
    for (const std::string& term : query_terms) {
        auto it = inverted_index_.find(term);
        if (it != inverted_index_.end()) {
            if (candidate_docs.empty()) {
                candidate_docs = it->second;
            } else {
                // 取交集（AND操作）或并集（OR操作）
                // 这里使用OR操作，可以根据需要修改
                std::set<std::string> temp;
                std::set_union(candidate_docs.begin(), candidate_docs.end(),
                              it->second.begin(), it->second.end(),
                              std::inserter(temp, temp.begin()));
                candidate_docs = temp;
            }
        }
    }

    // 计算每个候选文档的相关性分数
    std::vector<std::pair<std::string, double>> scored_docs;
    for (const std::string& doc_id : candidate_docs) {
        double score = calculate_relevance_score(doc_id, query_terms);
        if (score > 0) {
            scored_docs.push_back(std::make_pair(doc_id, score));
        }
    }

    // 按分数降序排序
    std::sort(scored_docs.begin(), scored_docs.end(),
              [](const std::pair<std::string, double>& a, const std::pair<std::string, double>& b) {
                  return a.second > b.second;
              });

    // 构建搜索结果
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

            // 截取内容摘要（前200个字节，但要确保不截断UTF-8字符）
            if (content.length() > 180) {
                size_t cut_pos = 180;
                // 确保不在UTF-8字符中间截断
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

void SearchEngine::build_index() {
    std::cout << "Starting to build index..." << std::endl;

    // 索引构建在add_document中完成
    // 这里可以进行一些优化操作

    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    std::cout << "Index build completed:" << std::endl;
    std::cout << "  Document count: " << documents_.size() << std::endl;
    std::cout << "  Vocabulary size: " << inverted_index_.size() << std::endl;
}

void SearchEngine::load_data_files(const std::string& data_dir) {
    std::cout << "Loading data files from directory: " << data_dir << std::endl;

    Indexer indexer;
    std::vector<Document> documents = indexer.scan_directory(data_dir);

    std::cout << "Found " << documents.size() << " documents" << std::endl;

    // 添加文档到索引
    for (const Document& doc : documents) {
        add_document(doc.id, doc.title, doc.content);
    }

    // 如果没有找到文档，添加一些示例数据
    if (documents.empty()) {
        std::cout << "No data files found, adding sample data..." << std::endl;

        add_document("doc1", "C++编程入门",
                    "C++是一种通用的编程语言，支持面向对象编程。它是C语言的扩展，"
                    "提供了类、对象、继承、多态等特性。C++广泛应用于系统软件、"
                    "游戏开发、嵌入式系统等领域。");

        add_document("doc2", "Boost库详细介绍",
                    "Boost库是为C++语言标准库提供扩展的一些C++程序库的总称。Boost库由Boost社区组织开发、维护。Boost库可以与C++标准库完美共同工作，并为其提供扩展功能。");

        add_document("doc3", "搜索引擎原理",
                    "搜索引擎的核心是倒排索引，它将词汇映射到包含该词汇的文档列表。"
                    "TF-IDF算法用于计算文档与查询的相关性。现代搜索引擎还使用机器学习"
                    "和深度学习技术来提高搜索质量。");

        add_document("doc4", "网络编程基础",
                    "网络编程涉及套接字编程、TCP/UDP协议、HTTP协议等。Boost.Asio"
                    "提供了异步网络编程的强大支持，可以构建高性能的网络应用程序。");

        add_document("doc5", "多线程编程",
                    "多线程编程可以提高程序的并发性能。需要注意线程安全、死锁、"
                    "竞态条件等问题。C++11引入了标准的线程库，Boost.Thread"
                    "提供了更丰富的线程功能。");
    }
}

double SearchEngine::calculate_tfidf(const std::string& term, const std::string& doc_id) {
    // 计算TF (Term Frequency)
    auto tf_it = term_frequency_.find(doc_id);
    if (tf_it == term_frequency_.end()) {
        return 0.0;
    }

    auto term_it = tf_it->second.find(term);
    if (term_it == tf_it->second.end()) {
        return 0.0;
    }

    double tf = static_cast<double>(term_it->second);

    // 计算文档总词数
    int total_terms = 0;
    for (const auto& pair : tf_it->second) {
        total_terms += pair.second;
    }

    tf = tf / total_terms;  // 归一化TF

    // 计算IDF (Inverse Document Frequency)
    auto df_it = document_frequency_.find(term);
    if (df_it == document_frequency_.end()) {
        return 0.0;
    }

    double df = static_cast<double>(df_it->second);
    double total_docs = static_cast<double>(documents_.size());
    double idf = std::log(total_docs / df);

    return tf * idf;
}

double SearchEngine::calculate_relevance_score(const std::string& doc_id, const std::vector<std::string>& query_terms) {
    double score = 0.0;

    for (const std::string& term : query_terms) {
        score += calculate_tfidf(term, doc_id);
    }

    // 可以添加其他相关性因子，如文档长度归一化、查询词位置等

    return score;
}

std::pair<std::string, std::string> SearchEngine::get_document(const std::string& doc_id) {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    auto it = documents_.find(doc_id);
    if (it != documents_.end()) {
        return it->second; // 返回 (title, content)
    }

    return std::make_pair("", ""); // 文档不存在
}
