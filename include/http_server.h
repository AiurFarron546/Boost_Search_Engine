#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <string>
#include <map>

using boost::asio::ip::tcp;

/**
 * HTTP连接类 - 处理单个HTTP连接
 */
class HttpConnection : public boost::enable_shared_from_this<HttpConnection>
{
public:
    typedef boost::shared_ptr<HttpConnection> pointer;

    static pointer create(boost::asio::io_service& io_service);

    tcp::socket& socket();

    void start();

private:
    HttpConnection(boost::asio::io_service& io_service);

    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error);

    std::string process_request(const std::string& request);
    std::string create_response(const std::string& content, const std::string& content_type = "text/html");
    std::string get_file_content(const std::string& file_path);
    std::string url_decode(const std::string& encoded);
    std::string escape_json(const std::string& str);
    std::string serve_document(const std::string& doc_id);
    std::string escape_html(const std::string& str);

    // 编码检测和转换函数
    std::string detect_and_convert_encoding(const std::string& raw_content);
    std::string detect_encoding(const std::string& content);
    std::string remove_bom(const std::string& content);
    std::string convert_gbk_to_utf8(const std::string& gbk_content);
    std::string simple_gbk_to_utf8(const std::string& gbk_content);

    tcp::socket socket_;
    enum { max_length = 8192 };
    char data_[max_length];
};

/**
 * HTTP服务器类 - 管理HTTP服务器
 */
class HttpServer
{
public:
    HttpServer(boost::asio::io_service& io_service, short port);

private:
    void start_accept();
    void handle_accept(HttpConnection::pointer new_connection, const boost::system::error_code& error);

    tcp::acceptor acceptor_;
};

#endif // HTTP_SERVER_H
