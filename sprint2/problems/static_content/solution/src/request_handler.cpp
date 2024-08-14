#include "request_handler.h"

namespace http_handler {

RequestHandler::RequestHandler(model::Game &game) : game_(game) {}

StringResponse
RequestHandler::MakeStringResponse(http::status status, std::string_view body,
                                   unsigned http_version, bool keep_alive,
                                   std::string_view content_type) {
    StringResponse response(status, http_version);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

StringResponse RequestHandler::MakeAllMapsResponse(StringRequest &&req) {
    return TextRespose(std::move(req), http::status::ok,
                       json_loader::GetAllMapsInfoAsJsonString(game_));
}

StringResponse RequestHandler::MakeCurrentMapResponse(StringRequest &&req) {
    std::string_view path = req.target();
    std::vector<std::string> path_segments;
    boost::split(path_segments, path, boost::is_any_of("/"));

    if (path_segments.size() < 5) {
        std::cerr << "Invalid URL format" << std::endl;
        return GetBadRequest(std::move(req));
    }

    std::string map_id{};
    if (!boost::conversion::detail::try_lexical_convert(path_segments[4],
                                                        map_id)) {
        std::cerr << "Invalid map ID in the URL" << std::endl;
        return GetBadRequest(std::move(req));
    }

    auto map_ptr = game_.FindMap(model::Map::Id(map_id));
    if (!map_ptr) {
        return TextRespose(
            std::move(req), http::status::not_found,
            R"({"code":"mapNotFound","message":"Map not found"})"sv);
    }

    return TextRespose(std::move(req), http::status::ok,
                       json_loader::GetMapInfoAsJsonString(*map_ptr));
}

StringResponse RequestHandler::GetBadRequest(StringRequest &&req) {
    return TextRespose(std::move(req), http::status::bad_request,
                       R"({"code":"badRequest","message":"Bad request"})"sv);
}

StringResponse RequestHandler::HandleGetRequest(StringRequest &&req) {
    auto path = req.target();
    if ("/api/v1/maps"sv == path) {
        return MakeAllMapsResponse(std::move(req));
    } else if (path.starts_with("/api/v1/maps/"sv)) {
        return MakeCurrentMapResponse(std::move(req));
    } else {

        return GetBadRequest(std::move(req));
    }
}

StringResponse RequestHandler::TextRespose(const StringRequest &req,
                                           http::status status,
                                           std::string_view text) {
    return MakeStringResponse(status, text, req.version(), req.keep_alive());
}

} // namespace http_handler
