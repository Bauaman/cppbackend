#pragma once

#include <boost/json.hpp>
#include <boost/date_time.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

namespace sys = boost::system;
using namespace std::literals;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)

namespace logger { 

void LogError(const sys::error_code& ec, std::string_view where);
void LogError(const std::exception& ex);
void LogExit(const int code, const std::exception* ex = nullptr);
void LogMessageInfo (const boost::json::value& add_data, const std::string message);
void LogMessageTrace (const boost::json::value& add_data, const std::string message);

class Logger {
public:

    static void Init();

private:
    static void MyFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

};


} //namespace logger