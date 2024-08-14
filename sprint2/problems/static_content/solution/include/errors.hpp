#pragma once

#include <iostream>
#include <string_view>

namespace error {

using std::string_view_literals::operator""sv;

template <typename ErrorCode>
inline void Report(ErrorCode ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

} // namespace error