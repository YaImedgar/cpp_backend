#pragma once

#include "http_server.hpp"
#include "json_loader.hpp"
#include "model_fwd.hpp"

#include <string_view>

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

class RequestHandler final {
  public:
    struct Context {
        model::Game &game;
        const std::string static_content_directory_path;
    };

  public:
    explicit RequestHandler(const Context &);

    RequestHandler(RequestHandler &&) = default;
    RequestHandler &operator=(RequestHandler &&) = default;
    ~RequestHandler() = default;

    StringResponse operator()(StringRequest &&req);

  private:
    RequestHandler() = delete;
    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

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

#define TEMPLATE_REQUEST_PREFIX                                                \
    template <typename Body, typename Allocator, typename Send>
#define REQUEST_TYPE http::request<Body, http::basic_fields<Allocator>>

class LoggingRequestHandler {
  private:
    static void LogRequest(const http_server::tcp::endpoint endpoint,
                           const StringRequest &);
    static void LogResponse(const std::chrono::milliseconds,
                            const StringResponse &);

  public:
    LoggingRequestHandler(RequestHandler &handler_) : decorated_(handler_) {}

    TEMPLATE_REQUEST_PREFIX
    void operator()([[maybe_unused]] const http_server::tcp::endpoint endpoint,
                    REQUEST_TYPE &&req, Send &&sender) {
        // Логируем запрос
        LogRequest(endpoint, req);

        // Получаем текущее время
        auto start_time = std::chrono::steady_clock::now();

        // Обрабатываем запрос
        auto resp = decorated_(std::forward<REQUEST_TYPE>(req));

        // Получаем время обработки запроса.
        auto response_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);

        // Логируем ответ
        LogResponse(response_time, resp);

        sender(std::move(resp));
    }

  private:
    RequestHandler &decorated_;
};

#undef TEMPLATE_REQUEST_PREFIX
#undef REQUEST_TYPE

} // namespace http_handler
