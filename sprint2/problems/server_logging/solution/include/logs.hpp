#pragma once

#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp> // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/expressions/keyword.hpp>
#include <boost/log/trivial.hpp> // для BOOST_LOG_TRIVIAL
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace json = boost::json;
namespace logging = boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

namespace logs {

void init();

} // namespace logs
