#pragma once
#include "http_server.h"
#include "json_loader.h"
#include "model.h"

#include <iostream>
#include <optional>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы
    // контента
};

#define TEMPLATE_REQUEST_PREFIX                                                \
    template <typename Body, typename Allocator, typename Send>
#define REQUEST_TYPE RequestType<Body, Allocator, Send>

class RequestHandler final {
    TEMPLATE_REQUEST_PREFIX
    using RequestType = http::request<Body, http::basic_fields<Allocator>>;

  public:
    RequestHandler() = delete;
    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    RequestHandler(RequestHandler &&) = default;
    RequestHandler &operator=(RequestHandler &&) = default;

    ~RequestHandler() = default;

    explicit RequestHandler(model::Game &game);

  public:
    StringResponse MakeStringResponse(
        http::status, std::string_view body, unsigned http_version,
        bool keep_alive,
        std::string_view content_type = ContentType::APPLICATION_JSON);
    StringResponse TextRespose(const StringRequest &, http::status,
                               std::string_view text);

    StringResponse GetBadRequest(StringRequest &&);
    StringResponse HandleGetRequest(StringRequest &&);
    StringResponse MakeAllMapsResponse(StringRequest &&);
    StringResponse MakeCurrentMapResponse(StringRequest &&);

    TEMPLATE_REQUEST_PREFIX
    void operator()(REQUEST_TYPE &&req, Send &&sender) {
        auto path = req.target();
        if (!path.starts_with("/api/")) {
            sender(TextRespose(
                std::forward<REQUEST_TYPE>(req), http::status::forbidden,
                R"({"code":"forbidden","message":"forbidden to use non API path"})"sv));
            return;
        }

        if (http::verb::get == req.method()) {
            sender(HandleGetRequest(std::forward<REQUEST_TYPE>(req)));
            return;
        }

        sender(GetBadRequest(std::forward<REQUEST_TYPE>(req)));
    }

  private:
    model::Game &game_;
};

#undef TEMPLATE_REQUEST_PREFIX
#undef REQUEST_TYPE

} // namespace http_handler
