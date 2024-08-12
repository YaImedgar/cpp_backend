#pragma once
#include "http_server.h"
#include "json_loader.h"
#include "model.h"

#include <iostream>
#include <optional>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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

class RequestHandler {
  public:
    explicit RequestHandler(model::Game &game) : game_{game} {}

    RequestHandler(const RequestHandler &) = delete;
    RequestHandler &operator=(const RequestHandler &) = delete;

    // Создаёт StringResponse с заданными параметрами
    StringResponse MakeStringResponse(
        http::status status, std::string_view body, unsigned http_version,
        bool keep_alive,
        std::string_view content_type = ContentType::APPLICATION_JSON) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    StringResponse MakeAllMapsResponse(StringRequest &&req) {
        return TextRespose(std::move(req), http::status::ok,
                           json_loader::GetAllMapsInfoAsJsonString(game_));
    }

    StringResponse MakeCurrentMapResponse(StringRequest &&req) {
        std::string_view path = req.target();
        std::vector<std::string> path_segments;
        boost::split(path_segments, path, boost::is_any_of("/"));

        std::optional<std::string> map_id;

        if (path_segments.size() >= 4) {
            try {
                map_id = boost::lexical_cast<std::string>(path_segments[4]);
            } catch (const boost::bad_lexical_cast &) {
                std::cerr << "Invalid map ID in the URL" << std::endl;
            }
        } else {
            std::cerr << "Invalid URL format" << std::endl;
        }
        if (!map_id) {
            return (GetBadRequest(std::move(req)));
        }

        auto map_ptr = game_.FindMap(model::Map::Id(map_id.value()));
        if (!map_ptr) {
            return TextRespose(
                std::move(req), http::status::not_found,
                R"({"code":"mapNotFound","message":"Map not found"})"sv);
        }

        return TextRespose(std::move(req), http::status::ok,
                           json_loader::GetMapInfoAsJsonString(*map_ptr));
    }

    StringResponse GetBadRequest(StringRequest &&req) {
        return TextRespose(
            std::move(req), http::status::bad_request,
            R"({"code":"badRequest","message":"Bad request"})"sv);
    }

    StringResponse HandleGetRequest(StringRequest &&req) {
        auto path = req.target();
        if ("/api/v1/maps"sv == path) {
            return MakeAllMapsResponse(std::move(req));
        } else if (path.starts_with("/api/v1/maps/"sv)) {
            return MakeCurrentMapResponse(std::move(req));
        } else {
            return GetBadRequest(std::move(req));
        }
    }

    StringResponse TextRespose(const StringRequest &req, http::status status,
                               std::string_view text) {
        return MakeStringResponse(status, text, req.version(),
                                  req.keep_alive());
    }
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>> &&req,
                    Send &&sender) {
        using RequestType = decltype(req);

        if (std::string_view path = req.target(); !path.starts_with("/api/")) {
            sender(TextRespose(
                std::forward<RequestType>(req), http::status::forbidden,
                R"({"code":"forbidden","message":"forbidden to use non API path"})"sv));
            return;
        }

        if (http::verb::get == req.method()) {
            sender(HandleGetRequest(std::forward<RequestType>(req)));
        } else {
            sender(GetBadRequest(std::forward<RequestType>(req)));
        }
    }

  private:
    model::Game &game_;
};

} // namespace http_handler
