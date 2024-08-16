#include <string_view>

#include "logs.hpp"

namespace error {

void Report(boost::system::error_code ec, std::string_view what) {
    json::object data;

    data["code"] = ec.value();
    data["text"] = std::move(ec.message());
    data["where"] = std::string(what);

    BOOST_LOG_TRIVIAL(info)
        << logging::add_value(additional_data, std::move(data)) << "error";
}

} // namespace error