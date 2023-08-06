#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;


struct HttpRequest
{
    std::string host;
    std::string port;
    std::string target;
    int version; // 10 or 11
    
    http::response<http::dynamic_body> Get() const
    {
        http::response<http::dynamic_body> response;

        boost::asio::io_context io_context;

        tcp::resolver resolver{ io_context };
        tcp::socket socket{ io_context };
        auto resolved = resolver.resolve(host, port);
        boost::asio::connect(socket, resolved.begin(), resolved.end());

        http::request<http::string_body> request{ http::verb::get, target, version };

        request.set(http::field::host, host);
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(socket, request);

        boost::beast::flat_buffer buffer;

        http::read(socket, buffer, response);

        boost::system::error_code error_code;
        socket.shutdown(tcp::socket::shutdown_both, error_code);

        if (error_code && error_code != boost::system::errc::not_connected)
            throw boost::beast::system_error{ error_code };

        return std::move(response);
    }

};