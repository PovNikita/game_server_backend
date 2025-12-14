#pragma once

#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/utility/string_view.hpp>

#include <string>
#include <filesystem>
#include <optional>
#include <variant>

namespace http_handler{

using namespace std::literals;

namespace beast = boost::beast;
namespace http = beast::http;

using StringRequest = http::request<http::string_body>;
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

using Response = std::variant<StringResponse, FileResponse>;

struct Endpoints {
    Endpoints() = delete;
    constexpr static std::string_view api_endpoint              = "/api"sv;
    constexpr static std::string_view version_endpoint          = "/api/v1"sv;
    constexpr static std::string_view maps_endpoint             = "/api/v1/maps"sv;
    constexpr static std::string_view join_endpoint             = "/api/v1/game/join"sv;
    constexpr static std::string_view get_players_in_session    = "/api/v1/game/players"sv;
    constexpr static std::string_view get_state                 = "/api/v1/game/state"sv;
    constexpr static std::string_view action                    = "/api/v1/game/player/action"sv;
    constexpr static std::string_view tick                      = "/api/v1/game/tick"sv;
    constexpr static std::string_view records                   = "/api/v1/game/records";
    
};

struct ContentType {
    ContentType() = delete;
    // Text / HTML
    constexpr static std::string_view TEXT_HTML         = "text/html"sv;
    constexpr static std::string_view TEXT_CSS          = "text/css"sv;
    constexpr static std::string_view TEXT_PLAIN        = "text/plain"sv;
    constexpr static std::string_view TEXT_JAVASCRIPT   = "text/javascript"sv;

    // Application
    constexpr static std::string_view APPLICATION_JSON  = "application/json"sv;
    constexpr static std::string_view APPLICATION_XML   = "application/xml"sv;

    // Image
    constexpr static std::string_view IMAGE_PNG     = "image/png"sv;
    constexpr static std::string_view IMAGE_JPEG    = "image/jpeg"sv;
    constexpr static std::string_view IMAGE_GIF     = "image/gif"sv;
    constexpr static std::string_view IMAGE_BMP     = "image/bmp"sv;
    constexpr static std::string_view IMAGE_ICON    = "image/vnd.microsoft.icon"sv;
    constexpr static std::string_view IMAGE_TIFF    = "image/tiff"sv;
    constexpr static std::string_view IMAGE_SVG     = "image/svg+xml"sv;

    // Audio
    constexpr static std::string_view AUDIO_MPEG = "audio/mpeg"sv;

    //Unknown
    constexpr static std::string_view UNKNOWN = "application/octet-stream"sv;
};

StringResponse FormErrorJsonResponse(const StringRequest& req, http::status status, 
                                            std::string_view code, std::string_view message, 
                                            std::string_view cache_control = ""sv, std::string_view allow = ""sv);

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, std::string_view content_type, 
                                std::string_view cache_control = ""sv, std::string_view allow = ""sv) ;
}

namespace utils
{
class URLDecoder {
public:
    URLDecoder() = delete;
    explicit URLDecoder(std::string_view URL_string);

    std::string ChangeURLCodedWordsToSymbols(const std::string_view str);

    std::string_view GetDecodedURLAsStringView() const noexcept;
    std::string GetDecodedURLAsString() const noexcept;
    std::filesystem::path GetDecodedURLAsPath();
private:
    std::string decoded_str_;
};

bool IsSubPath(std::filesystem::path path, std::filesystem::path base);
std::optional<std::string_view>ContentTypeByFileExtension(std::string_view file_extension);
}