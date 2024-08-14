#pragma once

#include <filesystem>

#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path &json_map_path);
std::string GetAllMapsInfoAsJsonString(const model::Game &game);
std::string GetMapInfoAsJsonString(const model::Map &map);

} // namespace json_loader
