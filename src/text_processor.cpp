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
    : word_pattern_("\\b\\w+\\b"),
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

    boost::sregex_iterator iter(text.begin(), text.end(), word_pattern_);
    boost::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string token = iter->str();
        if (token.length() >= 2) {  // 过滤太短的词
            tokens.push_back(token);
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

    // 中文停用词 (使用拼音表示以避免编码问题)
    std::vector<std::string> chinese_stop_words = {
        "de", "le", "zai", "shi", "wo", "you", "he", "jiu", "bu", "ren",
        "dou", "yi", "yige", "shang", "ye", "hen", "dao", "shuo", "yao",
        "qu", "ni", "hui", "zhe", "meiyou", "kan", "hao", "ziji", "zhe",
        "na", "li", "jiushi", "hai", "ba", "bi", "huozhe", "shenme",
        "keyi", "wei", "danshi", "zhege", "zhong", "lai", "yong", "ta",
        "ta2", "women", "neng", "xia", "zi", "dui", "ba2", "er", "bei",
        "zui", "gai", "xie", "you2", "jia", "ke", "yi2", "ruguo", "mei",
        "duo", "ranhou", "zenme", "chu", "ne", "yu", "qi", "gei", "cong",
        "shi2", "mei2", "ge", "xianzai", "rang", "yinwei", "dang", "tong",
        "hui2", "guo", "zhi", "xiang", "shiji", "hou", "zuo", "dian", "qi2",
        "san", "yu2", "guanyu"
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
