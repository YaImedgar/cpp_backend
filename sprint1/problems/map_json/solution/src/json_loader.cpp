#include "json_loader.h"

#include <fstream>
#include <iostream>
#include <optional>

#include <boost/json.hpp>

namespace json_loader {

model::Game LoadGame(const std::filesystem::path &json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла    // Прочитать содержимое файла
    std::ifstream file(json_path, std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_str = buffer.str();

    // Распарсить JSON
    boost::json::value json_value = boost::json::parse(json_str);

    // Загрузить модель игры из JSON
    model::Game game;

    for (auto &json_map : json_value.at("maps").as_array()) {
        model::Map::Id id(json_map.at("id").as_string().c_str());
        auto name = json_map.at("name").as_string();
        model::Map map(id, name.c_str());

        for (auto &json_road : json_map.at("roads").as_array()) {
            // RoadJson example
            // HORIZONTAL { "x0": 0, "y0": 0, "x1": 40 }
            // VERTICAL { "x0": 0, "y0": 0, "y1": 20 }
            model::Point start{
                .x = json_road.at("x0").as_int64(),
                .y = json_road.at("y0").as_int64(),
            };

            std::optional<model::Road> road;

            if (auto &obj = json_road.as_object(); obj.contains("x1")) {
                road = model::Road(model::Road::HORIZONTAL, start,
                                   json_road.at("x1").as_int64());
            } else if (obj.contains("y1")) {
                road = model::Road(model::Road::VERTICAL, start,
                                   json_road.at("y1").as_int64());
            } else {
                std::cerr << "access to incomplete json_road" << std::endl;
                std::abort();
            }

            map.AddRoad(std::move(road.value()));
        }

        if (json_map.as_object().contains("buildings")) {
            for (auto &json_building : json_map.at("buildings").as_array()) {
                // BuildingJson example
                // { "x": 5, "y": 5, "w": 30, "h": 20 }
                model::Rectangle bounds{
                    .position =
                        {
                            .x = json_building.at("x").as_int64(),
                            .y = json_building.at("y").as_int64(),
                        },
                    .size =
                        {
                            .width = json_building.at("w").as_int64(),
                            .height = json_building.at("h").as_int64(),
                        },
                };
                model::Building building(bounds);
                map.AddBuilding(std::move(building));
            }
        }

        if (json_map.as_object().contains("offices")) {
            for (auto &json_office : json_map.at("offices").as_array()) {
                // OfficeJson example
                // { "id": "o0", "x": 40, "y": 30, "offsetX": 5, "offsetY": 0 }
                model::Office::Id id(json_office.at("id").as_string().c_str());

                model::Point point{
                    .x = json_office.at("x").as_int64(),
                    .y = json_office.at("y").as_int64(),
                };

                model::Offset offset{
                    .dx = json_office.at("offsetX").as_int64(),
                    .dy = json_office.at("offsetY").as_int64(),
                };

                model::Office office(id, point, offset);

                map.AddOffice(std::move(office));
            }
        }

        game.AddMap(std::move(map));
    }

    return game;
}

std::string GetAllMapsInfoAsJsonString(const model::Game &game) {
    boost::json::array array;

    for (const auto &map : game.GetMaps()) {
        boost::json::object obj;
        obj["id"] = *map.GetId();
        obj["name"] = map.GetName();
        array.emplace_back(std::move(obj));
    }

    return boost::json::serialize(array);
}

std::string GetMapInfoAsJsonString(const model::Map &map) {
    boost::json::object json_map;

    json_map["id"] = *map.GetId();
    json_map["name"] = map.GetName();

    {
        boost::json::array roads;
        for (const auto &road : map.GetRoads()) {
            boost::json::object json_road;

            {
                auto [x, y] = road.GetStart();
                json_road["x0"] = x;
                json_road["y0"] = y;
            }

            auto [x, y] = road.GetEnd();

            if (road.IsHorizontal()) {
                json_road["x1"] = x;
            } else {
                json_road["y1"] = y;
            }

            roads.emplace_back(std::move(json_road));
        }

        json_map["roads"] = std::move(roads);
    }

    {
        boost::json::array buildings;
        for (const auto &building : map.GetBuildings()) {
            boost::json::object json_building;

            auto [position, size] = building.GetBounds();

            json_building["x"] = position.x;
            json_building["y"] = position.y;
            json_building["w"] = size.width;
            json_building["h"] = size.height;

            buildings.emplace_back(std::move(json_building));
        }

        json_map["buildings"] = std::move(buildings);
    }

    {
        boost::json::array offices;
        for (const auto &office : map.GetOffices()) {
            boost::json::object json_office;

            auto [x, y] = office.GetPosition();
            auto [dx, dy] = office.GetOffset();

            json_office["id"] = *office.GetId();
            json_office["x"] = x;
            json_office["y"] = y;
            json_office["offsetX"] = dx;
            json_office["offsetY"] = dy;

            offices.emplace_back(std::move(json_office));
        }

        json_map["offices"] = std::move(offices);
    }

    return boost::json::serialize(json_map);
}

} // namespace json_loader
