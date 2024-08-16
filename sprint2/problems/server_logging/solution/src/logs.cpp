#include "logs.hpp"

#include <boost/date_time.hpp>
#include <boost/log/core.hpp> // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <string_view>

namespace logs {

using namespace std::literals;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

void MyFormatter(logging::record_view const &rec,
                 logging::formatting_ostream &strm) {
    json::object obj;

    auto ts = *rec[timestamp];
    obj["timestamp"] = to_iso_extended_string(ts);
    if (auto data_ptr = rec[additional_data].get_ptr()) {
        obj["data"] = *data_ptr;
    }
    obj["message"] = rec[expr::smessage].get();

    strm << obj;
}

void init() {
    logging::add_common_attributes();

    logging::add_console_log(std::cout, keywords::format = &MyFormatter,
                             keywords::auto_flush = true);
}

} // namespace logs