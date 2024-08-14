#include "request_handler.hpp"
#include "errors.hpp"
#include "model.hpp"

#include <boost/algorithm/string.hpp>

#include <fstream>
#include <unordered_map>

namespace http_handler {

using namespace std::string_view_literals;

namespace {

StringResponse MakeStringResponse(http::status status, std::string_view body,
                                  unsigned http_version, bool keep_alive,
                                  std::string_view content_type) {
    StringResponse response(status, http_version);

    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);

    return response;
}

StringResponse TextRespose(const StringRequest &req, http::status status,
                           std::string_view text,
                           std::string_view content_type) {
    return MakeStringResponse(status, text, req.version(), req.keep_alive(),
                              content_type);
}

StringResponse GetBadRequest(StringRequest &&req) {
    return TextRespose(std::move(req), http::status::bad_request,
                       R"({"code":"badRequest","message":"Bad request"})"sv,
                       ContentType::APPLICATION_JSON);
}

} // namespace

RequestHandler::RequestHandler(const Context &c)
    : game_(c.game), content_root_(std::move(c.static_content_directory_path)) {
}

StringResponse RequestHandler::MakeAllMapsResponse(StringRequest &&req) {
    return TextRespose(std::move(req), http::status::ok,
                       json_loader::GetAllMapsInfoAsJsonString(game_),
                       ContentType::APPLICATION_JSON);
}

StringResponse RequestHandler::MakeCurrentMapResponse(StringRequest &&req) {
    std::string_view path = req.target();
    std::vector<std::string> path_segments;
    boost::split(path_segments, path, boost::is_any_of("/"));

    if (path_segments.size() < 5) {
        std::cerr << "Invalid URI format"sv << std::endl;
        return GetBadRequest(std::move(req));
    }

    std::string map_id{};
    if (!boost::conversion::detail::try_lexical_convert(path_segments[4],
                                                        map_id)) {
        std::cerr << "Invalid map ID in the URI"sv << std::endl;
        return GetBadRequest(std::move(req));
    }

    auto map_ptr = game_.FindMap(model::Map::Id(map_id));
    if (!map_ptr) {
        return TextRespose(
            std::move(req), http::status::not_found,
            R"({"code":"mapNotFound","message":"Map not found"})"sv,
            ContentType::APPLICATION_JSON);
    }

    return TextRespose(std::move(req), http::status::ok,
                       json_loader::GetMapInfoAsJsonString(*map_ptr),
                       ContentType::APPLICATION_JSON);
}

StringResponse RequestHandler::HandleGetRequest(StringRequest &&req) {
    auto path = req.target();
    if ("/api/v1/maps"sv == path) {
        return MakeAllMapsResponse(std::move(req));
    }

    if (path.starts_with("/api/v1/maps/"sv)) {
        return MakeCurrentMapResponse(std::move(req));
    }

    return GetBadRequest(std::move(req));
}

StringResponse RequestHandler::HandleApiRequest(StringRequest &&req) {
    switch (req.method()) {
        using enum http::verb;
    case get:
        return HandleGetRequest(std::move(req));
    case head:
        // FIX: TODO:
        return GetBadRequest(std::move(req));
    default:
        return GetBadRequest(std::move(req));
    }
}

namespace fs = std::filesystem;

std::string_view GetContentTypeByExtension(std::string extension) {
    boost::to_lower(extension);

    static const std::unordered_map<std::string_view, std::string_view>
        content_types = {
            {".htm"sv, ContentType::TEXT_HTML},
            {".html"sv, ContentType::TEXT_HTML},
            {".css"sv, ContentType::TEXT_CSS},
            {".txt"sv, ContentType::TEXT_PLAIN},
            {".js"sv, ContentType::TEXT_JAVASCRIPT},
            {".json"sv, ContentType::APPLICATION_JSON},
            {".xml"sv, ContentType::APPLICATION_XML},
            {".png"sv, ContentType::IMAGE_PNG},
            {".jpg"sv, ContentType::IMAGE_JPEG},
            {".jpe"sv, ContentType::IMAGE_JPEG},
            {".jpeg"sv, ContentType::IMAGE_JPEG},
            {".gif"sv, ContentType::IMAGE_GIF},
            {".bmp"sv, ContentType::IMAGE_BMP},
            {".ico"sv, ContentType::IMAGE_ICON},
            {".tiff"sv, ContentType::IMAGE_TIFF},
            {".tif"sv, ContentType::IMAGE_TIFF},
            {".svg"sv, ContentType::IMAGE_SVG},
            {".svgz"sv, ContentType::IMAGE_SVG},
            {".mp3"sv, ContentType::AUDIO_MPEG},
        };

    auto it = content_types.find(extension);
    if (it != content_types.end()) {
        return it->second;
    }

    return ContentType::APPLICATION_OCTET_STREAM;
}

using namespace std::string_literals;

std::string url_decode(const std::string_view str) {
    std::string result;
    result.reserve(str.size());

    // escape first '/'
    for (size_t i = 1; i < str.length(); i++) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex = 0;
            if (std::isxdigit(str[i + 1]) && std::isxdigit(str[i + 2])) {
                auto hex_str = str.substr(i + 1, 2);
                hex = std::stoi({hex_str.begin(), hex_str.size()}, nullptr, 16);
            }
            result += static_cast<char>(hex);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }

    return result;
}

StringResponse RequestHandler::ServeStaticFile(StringRequest &&req) {
    std::error_code ec{};

    auto path = url_decode(req.target());
    if (path.empty()) {
        path = "index.html"sv;
    }

    fs::path full_path = fs::canonical(fs::path(content_root_) / path, ec);

    if (ec || !fs::exists(full_path) || !fs::is_regular_file(full_path)) {
        error::Report(ec, "File not found: "s.append(path));
        return TextRespose(std::move(req), http::status::not_found,
                           "File not found"sv, ContentType::TEXT_PLAIN);
    }

    auto relative = fs::relative(full_path, content_root_);
    if (relative.empty()) {
        error::Report(ec, "File is not in " + content_root_);
        return GetBadRequest(std::move(req));
    }

    std::ifstream file(full_path.string(), std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    std::string_view content_type =
        GetContentTypeByExtension(full_path.extension());

    return MakeStringResponse(http::status::ok, content, req.version(),
                              req.keep_alive(), content_type);
}

} // namespace http_handler
