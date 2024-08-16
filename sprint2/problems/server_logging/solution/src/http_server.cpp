#include "http_server.hpp"
#include "errors.hpp"

#include <iostream>

namespace http_server {

using namespace std::string_view_literals;

void SessionBase::Run() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

SessionBase::SessionBase(tcp::socket &&socket) : stream_(std::move(socket)) {}

void SessionBase::Read() {
    using namespace std::literals;
    request_ = {};
    stream_.expires_after(30s);
    http::async_read(
        stream_, buffer_, request_,
        beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec,
                         [[maybe_unused]] std::size_t bytes_read) {
    using namespace std::literals;
    if (ec == http::error::end_of_stream) {
        return Close();
    }

    if (ec) {
        return error::Report(ec, "read"sv);
    }

    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

    if (ec) {
        return error::Report(ec, "read"sv);
    }
}

void SessionBase::OnWrite(bool close, beast::error_code ec,
                          [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        return error::Report(ec, "write"sv);
    }

    if (close) {
        return Close();
    }

    Read();
}

} // namespace http_server
