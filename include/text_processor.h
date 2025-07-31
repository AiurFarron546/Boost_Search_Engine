#ifndef TEXT_PROCESSOR_H
#define TEXT_PROCESSOR_H

#include <string>
#include <vector>
#include <set>
#include <boost/regex.hpp>

/**
 * 文本处理器类 - 负责文本预处理和分词
 */
class TextProcessor
{
public:
    TextProcessor();
    ~TextProcessor();
    
    // 文本预处理（清理、标准化）
    std::string preprocess_text(const std::string& text);
    
    // 分词处理
    std::vector<std::string> tokenize(const std::string& text);
    
    // 移除停用词
    std::vector<std::string> remove_stop_words(const std::vector<std::string>& tokens);
    
    // 词干提取（简单版本）
    std::string stem_word(const std::string& word);
    
    // 加载停用词列表
    void load_stop_words(const std::string& stop_words_file);
    
private:
    // 停用词集合
    std::set<std::string> stop_words_;
    
    // 正则表达式模式
    boost::regex word_pattern_;
    boost::regex html_tag_pattern_;
    boost::regex whitespace_pattern_;
    
    // 转换为小写
    std::string to_lower(const std::string& text);
    
    // 移除HTML标签
    std::string remove_html_tags(const std::string& text);
    
    // 移除特殊字符
    std::string remove_special_chars(const std::string& text);
    
    // 初始化默认停用词
    void init_default_stop_words();
};

#endif // TEXT_PROCESSOR_H