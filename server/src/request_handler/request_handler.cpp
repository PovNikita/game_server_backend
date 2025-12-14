#include "request_handler.h"
#include "../json_handler/json_loader.h"

#include <deque>

namespace http_handler {

Response RequestHandler::PrepareStaticFileResponse(StringRequest&& req)
{
    namespace sys = boost::system;
    utils::URLDecoder decoded_URL(req.target());
    std::string full_target_path_str(static_file_path_);
    full_target_path_str += decoded_URL.GetDecodedURLAsString();
    std::filesystem::path full_target_path(full_target_path_str);
    if(!utils::IsSubPath(full_target_path, static_file_path_))
    {
        return FormErrorJsonResponse(req, http::status::bad_request, "badRequest"sv, "Bad request"sv);
    }    
    http::response<http::file_body> response;
    response.version(req.version());
    response.result(http::status::ok);

    if(std::filesystem::is_directory(full_target_path) || decoded_URL.GetDecodedURLAsStringView() == "/"sv || decoded_URL.GetDecodedURLAsStringView() == "/index.html")
    {
        response.insert(http::field::content_type, ContentType::TEXT_HTML);
        http::file_body::value_type file;
        full_target_path_str = static_file_path_;
        full_target_path_str += "/index.html";
        if (sys::error_code ec; file.open(full_target_path_str.data(), beast::file_mode::read, ec), ec) {
        return MakeStringResponse(http::status::not_found, "file not found"s, req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
        }
        else
        {
            response.body() = std::move(file);
        }
        response.prepare_payload();
        return response;
    }
    else
    {
        if(utils::ContentTypeByFileExtension(full_target_path.extension().string()).has_value())
        {
            response.insert(http::field::content_type, utils::ContentTypeByFileExtension(full_target_path.extension().string()).value());
        }
        else
        {
            response.insert(http::field::content_type, ContentType::UNKNOWN);
        }
    }

    http::file_body::value_type file;
    
    if (sys::error_code ec; file.open(full_target_path_str.data(), beast::file_mode::read, ec), ec) {
        return MakeStringResponse(http::status::not_found, "file not found"s, req.version(), req.keep_alive(), ContentType::TEXT_PLAIN);
    }
    else
    {
        response.body() = std::move(file);
    }

    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    response.prepare_payload();
    return response;
}

Response RequestHandler::HandleStaticFileRequest(StringRequest&& req) {
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    Response response = PrepareStaticFileResponse(std::move(req));
    std::string content_type;
    int response_code;
    std::visit([&content_type, &response_code](auto& response){
        content_type = response[http::field::content_type];
        response_code = response.result_int(); 
    }, response);
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::chrono::nanoseconds dur = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    LogResponse({content_type, response_code}, dur);
    return response;
}

}  // namespace http_handler