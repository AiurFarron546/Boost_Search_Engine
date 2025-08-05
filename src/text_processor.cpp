/**
 * 文本处理器实现
 *
 * 功能：
 * 1. 文本预处理和清理
 * 2. 分词处理
 * 3. 停用词过滤
 * 4. 词干提取
 */

#include "text_processor.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>

TextProcessor::TextProcessor()
    : word_pattern_("([\\u4e00-\\u9fff]+|[a-zA-Z]+\\d*|\\d+)"),
      html_tag_pattern_("<[^>]*>"),
      whitespace_pattern_("\\s+") {

    // 初始化默认停用词
    init_default_stop_words();

    std::cout << "Text processor initialized, stop words count: " << stop_words_.size() << std::endl;
}

TextProcessor::~TextProcessor() {
    std::cout << "Text processor cleanup" << std::endl;
}

std::string TextProcessor::preprocess_text(const std::string& text) {
    std::string processed = text;

    // 移除HTML标签
    processed = remove_html_tags(processed);

    // 移除特殊字符（保留中文字符）
    processed = remove_special_chars(processed);

    // 标准化空白字符
    processed = boost::regex_replace(processed, whitespace_pattern_, " ");
    boost::trim(processed);

    return processed;
}

std::vector<std::string> TextProcessor::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    
    // 处理英文单词和数字
    boost::regex english_pattern("[a-zA-Z]+\\d*|\\d+");
    boost::sregex_iterator english_iter(text.begin(), text.end(), english_pattern);
    boost::sregex_iterator end;
    
    for (; english_iter != end; ++english_iter) {
        std::string token = english_iter->str();
        if (token.length() >= 2) {
            tokens.push_back(to_lower(token));
        }
    }
    
    // 处理中文字符 - 提取所有中文字符
    std::vector<std::string> chinese_chars;
    for (size_t i = 0; i < text.length(); ) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        
        // 检查是否是中文字符的开始（UTF-8编码）
        if ((c & 0xE0) == 0xE0) {  // 三字节UTF-8字符（大部分中文字符）
            if (i + 2 < text.length()) {
                std::string chinese_char = text.substr(i, 3);
                chinese_chars.push_back(chinese_char);
                i += 3;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }
    
    // 将中文字符组合成不同长度的词
    for (size_t i = 0; i < chinese_chars.size(); ++i) {
        // 单个中文字符
        tokens.push_back(chinese_chars[i]);
        
        // 两字组合
        if (i + 1 < chinese_chars.size()) {
            tokens.push_back(chinese_chars[i] + chinese_chars[i+1]);
        }
        
        // 三字组合
        if (i + 2 < chinese_chars.size()) {
            tokens.push_back(chinese_chars[i] + chinese_chars[i+1] + chinese_chars[i+2]);
        }
        
        // 四字组合
        if (i + 3 < chinese_chars.size()) {
            tokens.push_back(chinese_chars[i] + chinese_chars[i+1] + chinese_chars[i+2] + chinese_chars[i+3]);
        }
    }
    
    return tokens;
}

std::vector<std::string> TextProcessor::remove_stop_words(const std::vector<std::string>& tokens) {
    std::vector<std::string> filtered_tokens;

    for (const std::string& token : tokens) {
        if (stop_words_.find(token) == stop_words_.end()) {
            filtered_tokens.push_back(token);
        }
    }

    return filtered_tokens;
}

std::string TextProcessor::stem_word(const std::string& word) {
    std::string stemmed = word;

    // 简单的英文词干提取规则
    if (stemmed.length() > 4) {
        // 移除常见后缀
        if (boost::ends_with(stemmed, "ing")) {
            stemmed = stemmed.substr(0, stemmed.length() - 3);
        } else if (boost::ends_with(stemmed, "ed")) {
            stemmed = stemmed.substr(0, stemmed.length() - 2);
        } else if (boost::ends_with(stemmed, "er")) {
            stemmed = stemmed.substr(0, stemmed.length() - 2);
        } else if (boost::ends_with(stemmed, "ly")) {
            stemmed = stemmed.substr(0, stemmed.length() - 2);
        }
    }

    return stemmed;
}

void TextProcessor::load_stop_words(const std::string& stop_words_file) {
    std::ifstream file(stop_words_file.c_str());
    if (!file.is_open()) {
        std::cerr << "Cannot open stop words file: " << stop_words_file << std::endl;
        return;
    }

    std::string word;
    while (std::getline(file, word)) {
        boost::trim(word);
        if (!word.empty()) {
            stop_words_.insert(to_lower(word));
        }
    }

    std::cout << "Loaded stop words from file: " << stop_words_file << std::endl;
}

std::string TextProcessor::to_lower(const std::string& text) {
    std::string lower_text = text;
    boost::to_lower(lower_text);
    return lower_text;
}

std::string TextProcessor::remove_html_tags(const std::string& text) {
    return boost::regex_replace(text, html_tag_pattern_, " ");
}

std::string TextProcessor::remove_special_chars(const std::string& text) {
    std::string result;
    result.reserve(text.length());

    for (size_t i = 0; i < text.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);

        // 保留ASCII字母数字和空格
        if (std::isalnum(c) || std::isspace(c) || c == '-' || c == '_') {
            result += c;
        }
        // 保留UTF-8中文字符（多字节字符）
        else if (c >= 0x80) {
            result += c;
        }
        // 其他字符替换为空格
        else {
            result += ' ';
        }
    }

    return result;
}

void TextProcessor::init_default_stop_words() {
    // 英文停用词
    std::vector<std::string> english_stop_words = {
        "a", "an", "and", "are", "as", "at", "be", "by", "for", "from",
        "has", "he", "in", "is", "it", "its", "of", "on", "that", "the",
        "to", "was", "will", "with", "the", "this", "but", "they", "have",
        "had", "what", "said", "each", "which", "she", "do", "how", "their",
        "if", "up", "out", "many", "then", "them", "these", "so", "some",
        "her", "would", "make", "like", "into", "him", "time", "two", "more",
        "go", "no", "way", "could", "my", "than", "first", "been", "call",
        "who", "oil", "sit", "now", "find", "down", "day", "did", "get",
        "come", "made", "may", "part"
    };

    // 中文停用词
    std::vector<std::string> chinese_stop_words = {
        "的", "了", "在", "是", "我", "有", "和", "就", "不", "人",
        "都", "一", "一个", "上", "也", "很", "到", "说", "要",
        "去", "你", "会", "着", "没有", "看", "好", "自己", "这",
        "那", "里", "就是", "还", "把", "比", "或者", "什么",
        "可以", "为", "但是", "这个", "中", "来", "用", "他",
        "她", "我们", "能", "下", "子", "对", "吧", "而", "被",
        "最", "该", "些", "又", "家", "可", "以", "如果", "没",
        "多", "然后", "怎么", "出", "呢", "与", "其", "给", "从",
        "时", "每", "个", "现在", "让", "因为", "当", "同",
        "回", "过", "只", "想", "实际", "后", "做", "点", "起",
        "三", "于", "关于"
    };

    // 添加英文停用词
    for (const std::string& word : english_stop_words) {
        stop_words_.insert(word);
    }

    // 添加中文停用词
    for (const std::string& word : chinese_stop_words) {
        stop_words_.insert(word);
    }

    // 添加数字和单字符
    for (int i = 0; i <= 9; ++i) {
        stop_words_.insert(std::to_string(i));
    }

    for (char c = 'a'; c <= 'z'; ++c) {
        stop_words_.insert(std::string(1, c));
    }
}
