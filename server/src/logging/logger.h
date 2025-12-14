#pragma once
//std
#include <string>
#include <utility>
#include <chrono>

//boost
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/attributes/constant.hpp>

#include <boost/date_time.hpp>

#include <boost/json.hpp>

namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace logging = boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(data, "data", boost::json::value);
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void MyFormatter(logging::record_view const& rec,  logging::formatting_ostream& strm);

void InitLogger();

void LogJson(const boost::json::value &data, std::string_view message);

void LogJsonThreadSafe(const boost::json::value& data, const std::string &message);

void LogResponse(const std::pair<std::string, int> &content_type_result, std::chrono::nanoseconds dur);