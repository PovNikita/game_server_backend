#pragma once
#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <optional>
#include <any>

#include "../logging/logger.h"
#include "../application/application.h"
#include "../server/http_server.h"
#include "../game/game.h"
#include "request_utils.h"
#include "api_handler.h"


namespace http_handler {

template<class SomeRequestHandler>
class RequestHandlerWithLogger
{
public:
    explicit RequestHandlerWithLogger(std::shared_ptr<SomeRequestHandler> handler)
        : decorated_handler{handler} {
    }

    static void LogRequest(const http_handler::StringRequest& request, boost::asio::ip::tcp::endpoint endpoint)
    {
        boost::json::value data = {
            {"ip", endpoint.address().to_string()},
            {"URI", request.target()},
            {"method", request.method_string()}
        };
        LogJson(data, "request received"sv);
    }

    template <typename Body, typename Allocator, typename Send>
    void operator () (boost::asio::ip::tcp::endpoint endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::string content_type = "";
        int return_code = 0;
        LogRequest(req, endpoint);
        (*decorated_handler)(endpoint, std::move(req), std::move(send));
    }

private:
    std::shared_ptr<SomeRequestHandler> decorated_handler;
};

class RequestHandler : public std::enable_shared_from_this<RequestHandler>{
public:
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;
    explicit RequestHandler(model::Game& game, app::Application& application, Strand& api_strand)
        : game_{game} 
        , api_handler_(game, application)
        , api_strand_(api_strand)
        , static_file_path_(game_.GetPathToStaticFiles()) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    RequestHandler(RequestHandler&& other) = default;
    RequestHandler& operator=(RequestHandler&& other) = default;

    template <typename Body, typename Allocator, typename Send>
    void operator()(boost::asio::ip::tcp::endpoint endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        if (api_handler_.IsApiRequest(req)) {
            auto handle = [self = shared_from_this(), send,
                            req = std::forward<decltype(req)>(req)] {
                try {
                    // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                    assert(self->api_strand_.running_in_this_thread());
                    http::request<Body, http::basic_fields<Allocator>>& req_to_move = const_cast<http::request<Body, http::basic_fields<Allocator>>&>(req);
                    auto response = self->api_handler_.HandleApiRequest(std::move(req_to_move));
                    return send(std::move(response));
                } catch (std::exception& ex) {
                    auto response = FormErrorJsonResponse(req, http::status::internal_server_error, /*"internalError"sv*/ ex.what(), "internalServerError"sv);
                    return send(std::move(response));
                }
            };
            return boost::asio::dispatch(api_strand_, handle);
        }
        // Возвращаем результат обработки запроса к файлу
        auto file_response = HandleStaticFileRequest(std::move(req));

        return std::visit([&send](auto&& resp) {
                    send(std::move(resp));
                }, std::move(file_response));
    }

private:
    Response HandleStaticFileRequest(StringRequest&& req);
    Response PrepareStaticFileResponse(StringRequest&& req);

    model::Game& game_;
    ApiHandler api_handler_;
    std::string static_file_path_;
    Strand& api_strand_;
};

std::string GetAllMapsInJSON(const model::Game& game);
std::optional<std::string> GetMapConfigurationJSON(const model::Game& game, const std::string& map_id);

}  // namespace http_handler

