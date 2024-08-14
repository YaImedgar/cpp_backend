#pragma once
#include "http_server.hpp"
#include "json_loader.hpp"
#include "model_fwd.hpp"

#include <iostream>
#include <optional>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>

namespace http_handler {

namespace http = boost::beast::http;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

using std::string_view_literals::operator""sv;

struct ContentType {
    ContentType() = delete;
    constexpr static auto TEXT_HTML = "text/html"sv;
    constexpr static auto APPLICATION_JSON = "application/json"sv;
    constexpr static auto TEXT_CSS = "text/css"sv;
    constexpr static auto TEXT_PLAIN = "text/plain"sv;
    constexpr static auto TEXT_JAVASCRIPT = "text/javascript"sv;
    constexpr static auto APPLICATION_XML = "application/xml"sv;
    constexpr static auto IMAGE_PNG = "image/png"sv;
    constexpr static auto IMAGE_JPEG = "image/jpeg"sv;
    constexpr static auto IMAGE_GIF = "image/gif"sv;
    constexpr static auto IMAGE_BMP = "image/bmp"sv;
    constexpr static auto IMAGE_ICON = "image/vnd.microsoft.icon"sv;
    constexpr static auto IMAGE_TIFF = "image/tiff"sv;
    constexpr static auto IMAGE_SVG = "image/svg+xml"sv;
    constexpr static auto AUDIO_MPEG = "audio/mpeg"sv;
    constexpr static auto APPLICATION_OCTET_STREAM =
        "application/octet-stream"sv;
};

#define TEMPLATE_REQUEST_PREFIX                                                \
    template <typename Body, typename Allocator, typename Send>
#define REQUEST_TYPE RequestType<Body, Allocator, Send>

class RequestHandler final {
    TEMPLATE_REQUEST_PREFIX
    using RequestType = http::request<Body, http::basic_fields<Allocator>>;

  public:
    struct Context {
        model::Game &game;
        const std::string static_content_directory_path;
    };

    RequestHandler() = delete;
    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    RequestHandler(RequestHandler &&) = default;
    RequestHandler &operator=(RequestHandler &&) = default;

    ~RequestHandler() = default;

    explicit RequestHandler(const Context &);

  public:
    TEMPLATE_REQUEST_PREFIX
    void operator()(REQUEST_TYPE &&req, Send &&sender) {
        if (req.target().starts_with("/api/")) {
            sender(HandleApiRequest(std::forward<REQUEST_TYPE>(req)));
            return;
        }

        sender(ServeStaticFile(std::forward<REQUEST_TYPE>(req)));
    }

  private:
    StringResponse HandleApiRequest(StringRequest &&);
    StringResponse ServeStaticFile(StringRequest &&);

    StringResponse HandleGetRequest(StringRequest &&);
    StringResponse MakeAllMapsResponse(StringRequest &&);
    StringResponse MakeCurrentMapResponse(StringRequest &&);

  private:
    model::Game &game_;
    const std::string content_root_;
};

#undef TEMPLATE_REQUEST_PREFIX
#undef REQUEST_TYPE

} // namespace http_handler
