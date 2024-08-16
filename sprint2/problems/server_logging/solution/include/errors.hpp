#pragma once

#include <string_view>

#include <boost/system/error_code.hpp>

namespace error {

void Report(boost::system::error_code ec, std::string_view what);

} // namespace error