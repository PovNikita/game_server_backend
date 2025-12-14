#include "request_utils.h"

namespace http_handler{
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                bool keep_alive, std::string_view content_type, 
                                std::string_view cache_control, std::string_view allow) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    if(cache_control.size() != 0)
    {
        response.set(http::field::cache_control, cache_control);
    }
    if(allow.size() != 0)
    {
        response.set(http::field::allow, allow);
    }
    return response;
}

StringResponse FormErrorJsonResponse(const StringRequest& req, http::status status, 
                                            std::string_view code, std::string_view message, 
                                            std::string_view cache_control, std::string_view allow) {
    boost::json::object error_json{
        {"code", code},
        {"message", message}};
    return MakeStringResponse(status, boost::json::serialize(error_json), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, cache_control, allow);
}

}

namespace utils
{
URLDecoder::URLDecoder(std::string_view URL_string)
{
    decoded_str_ = ChangeURLCodedWordsToSymbols(URL_string);
}

std::string URLDecoder::ChangeURLCodedWordsToSymbols(const std::string_view str)
{
    using namespace std::literals;
    if(str.size() == 0) 
    {
        return ""s;
    }
    std::string decoded_str = ""s;
    for(size_t i = 0; i < str.size(); ++i)
    {
        if(str[i] == '%' && (i+2) < str.size())
        {
            int first_number = static_cast<int>(str[i+1]);
            int second_number = static_cast<int>(str[i+2]);
            int decoded_number = 0;
            if(((first_number <= '9' && first_number >= '0') || (first_number <= 'F' && first_number >= 'A')) &&
                ((second_number <= '9' && second_number >= '0') || (second_number <= 'F' && second_number >= 'A')))
            {
                char ch = static_cast<char>(std::stoi(std::string(str.begin() + (i + 1), str.begin() + i + 3), nullptr, 16));
                decoded_str.push_back(ch);
            }
        }
        else {
            decoded_str.push_back(str[i]);
        }
    }
    return decoded_str;
}

std::string_view URLDecoder::GetDecodedURLAsStringView() const noexcept
{
    return decoded_str_;
}

std::string URLDecoder::GetDecodedURLAsString() const noexcept
{
    return decoded_str_;
}

std::filesystem::path URLDecoder::GetDecodedURLAsPath()
{
    return std::filesystem::path(decoded_str_);
}

bool IsSubPath(std::filesystem::path path, std::filesystem::path base) {
    path = std::filesystem::weakly_canonical(path);
    base = std::filesystem::weakly_canonical(base);

    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::optional<std::string_view>ContentTypeByFileExtension(std::string_view file_extension)
{
    using namespace std::literals;
    const static std::unordered_map<std::string_view, std::string_view> extension_to_content_type = {
        {".htm"sv,     http_handler::ContentType::TEXT_HTML},
        {".html"sv,    http_handler::ContentType::TEXT_HTML},
        {".css"sv,     http_handler::ContentType::TEXT_CSS},
        {".txt"sv,     http_handler::ContentType::TEXT_PLAIN},
        {".js"sv,      http_handler::ContentType::TEXT_JAVASCRIPT},
        {".json"sv,    http_handler::ContentType::APPLICATION_JSON},
        {".xml"sv,     http_handler::ContentType::APPLICATION_XML},
        {".png"sv,     http_handler::ContentType::IMAGE_PNG},
        {".jpg"sv,     http_handler::ContentType::IMAGE_JPEG},
        {".jpe"sv,     http_handler::ContentType::IMAGE_JPEG},
        {".jpeg"sv,    http_handler::ContentType::IMAGE_JPEG},
        {".gif"sv,     http_handler::ContentType::IMAGE_GIF},
        {".bmp"sv,     http_handler::ContentType::IMAGE_BMP},
        {".ico"sv,     http_handler::ContentType::IMAGE_ICON},
        {".tiff"sv,    http_handler::ContentType::IMAGE_TIFF},
        {".tif"sv,     http_handler::ContentType::IMAGE_TIFF},
        {".svg"sv,     http_handler::ContentType::IMAGE_SVG},
        {".svgz"sv,    http_handler::ContentType::IMAGE_SVG},
        {".mp3"sv,     http_handler::ContentType::AUDIO_MPEG}
    };
    if(extension_to_content_type.find(file_extension) != extension_to_content_type.end())
    {
        return extension_to_content_type.at(file_extension);
    }
    return std::nullopt;
}
}