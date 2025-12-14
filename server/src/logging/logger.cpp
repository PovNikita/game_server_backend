#include "logger.h"
#include <chrono>

void MyFormatter(logging::record_view const& rec,  logging::formatting_ostream& strm)
{
    if(!rec[timestamp].empty())
    {
        auto ts = *rec[timestamp];
        strm << R"({"timestamp":")" << to_iso_extended_string(ts) << "\"";
    }
    if(!rec[data].empty())
    {
        strm << ",\"data\":" << boost::json::serialize(rec[data].get());
    }
    
    strm << R"(,"message":")"<< rec[logging::expressions::smessage] << "\"}";
}

void InitLogger()
{
    logging::add_common_attributes(); 

    logging::add_console_log(
        std::cout,
        keywords::auto_flush = true,
        keywords::format = &MyFormatter
    );
}

void LogJson(const boost::json::value &data, std::string_view message)
{
    BOOST_LOG_SCOPED_THREAD_TAG("data", data);
    BOOST_LOG_TRIVIAL(info) << message;
}

void LogJsonThreadSafe(const boost::json::value& data, const std::string &message)
{
    BOOST_LOG_SCOPED_THREAD_TAG("data", data);
    BOOST_LOG_TRIVIAL(info) << message;
}

void LogResponse(const std::pair<std::string, int> &content_type_result, std::chrono::nanoseconds dur)
{
    using namespace std::literals;
    boost::json::value data = {
        {"response_time", std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()},
        {"code", content_type_result.second},
        {"content_type", (!content_type_result.first.empty()) ? content_type_result.first : "null"}
    };
    LogJson(data, "response sent"sv);
}