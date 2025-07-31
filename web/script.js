/**
 * Boost搜索引擎 - 前端JavaScript
 *
 * 功能：
 * 1. 处理用户搜索交互
 * 2. 发送搜索请求到后端API
 * 3. 显示搜索结果
 * 4. 提供搜索建议
 */

class SearchEngine {
    constructor() {
        this.searchInput = document.getElementById('searchInput');
        this.searchButton = document.getElementById('searchButton');
        this.resultsSection = document.getElementById('resultsSection');
        this.resultsList = document.getElementById('resultsList');
        this.resultsCount = document.getElementById('resultsCount');
        this.searchTime = document.getElementById('searchTime');
        this.loading = document.getElementById('loading');
        this.suggestions = document.getElementById('suggestions');

        this.initEventListeners();
    }

    /**
     * 初始化事件监听器
     */
    initEventListeners() {
        // 搜索按钮点击事件
        this.searchButton.addEventListener('click', () => {
            this.performSearch();
        });

        // 搜索框回车事件
        this.searchInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.performSearch();
            }
        });

        // 搜索框输入事件（用于搜索建议）
        this.searchInput.addEventListener('input', (e) => {
            this.handleInputChange(e.target.value);
        });

        // 点击其他地方隐藏建议
        document.addEventListener('click', (e) => {
            if (!this.suggestions.contains(e.target) && e.target !== this.searchInput) {
                this.hideSuggestions();
            }
        });
    }

    /**
     * 执行搜索
     */
    async performSearch() {
        const query = this.searchInput.value.trim();

        if (!query) {
            alert('请输入搜索关键词');
            return;
        }

        this.showLoading();
        this.hideSuggestions();

        const startTime = Date.now();

        try {
            const results = await this.searchAPI(query);
            const endTime = Date.now();
            const searchTime = (endTime - startTime) / 1000;

            this.displayResults(results, searchTime);
        } catch (error) {
            console.error('搜索失败:', error);
            this.showError('搜索失败，请稍后重试');
        } finally {
            this.hideLoading();
        }
    }

    /**
     * 调用搜索API
     */
    async searchAPI(query) {
        const encodedQuery = encodeURIComponent(query);
        const response = await fetch(`/api/search?q=${encodedQuery}`);

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        console.log('API响应数据:', data);

        if (data.error) {
            throw new Error(data.error);
        }

        return data.results || [];
    }

    /**
     * 显示搜索结果
     */
    displayResults(results, searchTime) {
        this.resultsCount.textContent = `找到 ${results.length} 个结果`;
        this.searchTime.textContent = `用时 ${searchTime.toFixed(3)} 秒`;

        if (results.length === 0) {
            this.resultsList.innerHTML = `
                <div class="no-results">
                    <h3>未找到相关结果</h3>
                    <p>请尝试使用不同的关键词或检查拼写</p>
                </div>
            `;
        } else {
            this.resultsList.innerHTML = results.map(result =>
                this.createResultHTML(result)
            ).join('');
        }

        this.resultsSection.style.display = 'block';
        this.resultsSection.scrollIntoView({ behavior: 'smooth' });
    }

    /**
     * 创建单个搜索结果的HTML
     */
    createResultHTML(result) {
        const title = this.escapeHtml(result.title);
        const content = this.escapeHtml(result.content);
        const url = this.escapeHtml(result.url);
        const score = result.score.toFixed(3);

        return `
            <div class="result-item">
                <a href="${url}" class="result-title" target="_blank">${title}</a>
                <div class="result-content">${content}</div>
                <div class="result-meta">
                    <a href="${url}" class="result-url" target="_blank">${url}</a>
                    <span class="result-score">相关度: ${score}</span>
                </div>
            </div>
        `;
    }

    /**
     * 处理输入变化（搜索建议）
     */
    handleInputChange(value) {
        if (value.length < 2) {
            this.hideSuggestions();
            return;
        }

        // 这里可以实现搜索建议功能
        // 暂时显示一些示例建议
        const suggestions = this.generateSuggestions(value);
        this.showSuggestions(suggestions);
    }

    /**
     * 生成搜索建议
     */
    generateSuggestions(query) {
        // 示例建议，实际应用中可以从服务器获取
        const commonQueries = [
            'C++ 教程',
            'Boost 库使用',
            '搜索引擎原理',
            '倒排索引',
            'TF-IDF 算法',
            '文本处理',
            '网络编程',
            '多线程编程'
        ];

        return commonQueries
            .filter(suggestion =>
                suggestion.toLowerCase().includes(query.toLowerCase())
            )
            .slice(0, 5);
    }

    /**
     * 显示搜索建议
     */
    showSuggestions(suggestions) {
        if (suggestions.length === 0) {
            this.hideSuggestions();
            return;
        }

        this.suggestions.innerHTML = suggestions.map(suggestion => `
            <div class="suggestion-item" onclick="searchEngine.selectSuggestion('${suggestion}')">
                ${this.escapeHtml(suggestion)}
            </div>
        `).join('');

        this.suggestions.style.display = 'block';
    }

    /**
     * 隐藏搜索建议
     */
    hideSuggestions() {
        this.suggestions.style.display = 'none';
    }

    /**
     * 选择搜索建议
     */
    selectSuggestion(suggestion) {
        this.searchInput.value = suggestion;
        this.hideSuggestions();
        this.performSearch();
    }

    /**
     * 显示加载动画
     */
    showLoading() {
        this.loading.style.display = 'block';
        this.resultsSection.style.display = 'none';
    }

    /**
     * 隐藏加载动画
     */
    hideLoading() {
        this.loading.style.display = 'none';
    }

    /**
     * 显示错误信息
     */
    showError(message) {
        this.resultsList.innerHTML = `
            <div class="error-message">
                <h3>出现错误</h3>
                <p>${this.escapeHtml(message)}</p>
            </div>
        `;
        this.resultsSection.style.display = 'block';
    }

    /**
     * HTML转义
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// 页面加载完成后初始化搜索引擎
document.addEventListener('DOMContentLoaded', () => {
    window.searchEngine = new SearchEngine();

    // 添加一些样式增强
    addStyleEnhancements();
});

/**
 * 添加样式增强
 */
function addStyleEnhancements() {
    // 为无结果状态添加样式
    const style = document.createElement('style');
    style.textContent = `
        .no-results {
            text-align: center;
            padding: 60px 20px;
            color: #666;
        }

        .no-results h3 {
            font-size: 1.5rem;
            margin-bottom: 15px;
            color: #333;
        }

        .error-message {
            text-align: center;
            padding: 60px 20px;
            color: #dc3545;
        }

        .error-message h3 {
            font-size: 1.5rem;
            margin-bottom: 15px;
        }

        /* 搜索框焦点效果 */
        #searchInput:focus {
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.2);
        }

        /* 结果项动画 */
        .result-item {
            animation: fadeInUp 0.5s ease forwards;
            opacity: 0;
            transform: translateY(20px);
        }

        @keyframes fadeInUp {
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }

        /* 为每个结果项添加延迟动画 */
        .result-item:nth-child(1) { animation-delay: 0.1s; }
        .result-item:nth-child(2) { animation-delay: 0.2s; }
        .result-item:nth-child(3) { animation-delay: 0.3s; }
        .result-item:nth-child(4) { animation-delay: 0.4s; }
        .result-item:nth-child(5) { animation-delay: 0.5s; }
    `;
    document.head.appendChild(style);
}
