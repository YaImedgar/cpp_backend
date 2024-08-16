#include "json_loader.hpp"
#include "model.hpp"

#include <fstream>
#include <iostream>
#include <optional>

#include <boost/json.hpp>

namespace json_loader {

namespace constants {

inline boost::json::string_view X = "x";
inline boost::json::string_view X1 = "x1";
inline boost::json::string_view X0 = "x0";
inline boost::json::string_view Y = "y";
inline boost::json::string_view Y1 = "y1";
inline boost::json::string_view Y0 = "y0";
inline boost::json::string_view W = "w";
inline boost::json::string_view H = "h";
inline boost::json::string_view ID = "id";
inline boost::json::string_view MAPS = "maps";
inline boost::json::string_view NAME = "name";
inline boost::json::string_view ROADS = "roads";
inline boost::json::string_view BUILDINGS = "buildings";
inline boost::json::string_view OFFICES = "offices";
inline boost::json::string_view OFFSET_X = "offsetX";
inline boost::json::string_view OFFSET_Y = "offsetY";

} // namespace constants

namespace CONST = constants;

model::Map GetBasicMapData(const boost::json::value &json_map) {
    model::Map::Id id(json_map.at(CONST::ID).as_string().c_str());
    auto name = json_map.at(CONST::NAME).as_string();

    return model::Map(id, name.c_str());
}

void FillRoads(model::Map &map, const boost::json::value &json_map) {
    for (auto &json_road : json_map.at(CONST::ROADS).as_array()) {
        // RoadJson example
        // HORIZONTAL { "x0": 0, "y0": 0, "x1": 40 }
        // VERTICAL { "x0": 0, "y0": 0, "y1": 20 }
        model::Point start{
            .x = static_cast<int>(json_road.at(CONST::X0).as_int64()),
            .y = static_cast<int>(json_road.at(CONST::Y0).as_int64()),
        };

        std::optional<model::Road> road;

        if (auto &obj = json_road.as_object(); obj.contains(CONST::X1)) {
            road = model::Road(
                model::Road::HORIZONTAL, start,
                static_cast<int>(json_road.at(CONST::X1).as_int64()));
        } else if (obj.contains(CONST::Y1)) {
            road = model::Road(
                model::Road::VERTICAL, start,
                static_cast<int>(json_road.at(CONST::Y1).as_int64()));
        } else {
            std::cerr << "access to incomplete json_road" << std::endl;
            std::abort();
        }

        map.AddRoad(std::move(road.value()));
    }
}

void FillBuildings(model::Map &map, const boost::json::value &json_map) {
    if (json_map.as_object().contains(CONST::BUILDINGS)) {
        for (auto &json_building : json_map.at(CONST::BUILDINGS).as_array()) {
            // BuildingJson example
            // { "x": 5, "y": 5, "w": 30, "h": 20 }
            model::Rectangle bounds{
                .position =
                    {
                        .x = static_cast<int>(
                            json_building.at(CONST::X).as_int64()),
                        .y = static_cast<int>(
                            json_building.at(CONST::Y).as_int64()),
                    },
                .size =
                    {
                        .width = static_cast<int>(
                            json_building.at(CONST::W).as_int64()),
                        .height = static_cast<int>(
                            json_building.at(CONST::H).as_int64()),
                    },
            };
            model::Building building(bounds);
            map.AddBuilding(std::move(building));
        }
    }
}

void FillOffices(model::Map &map, const boost::json::value &json_map) {
    if (json_map.as_object().contains(CONST::OFFICES)) {
        for (auto &json_office : json_map.at(CONST::OFFICES).as_array()) {
            // OfficeJson example
            // { "id": "o0", "x": 40, "y": 30, "offsetX": 5, "offsetY": 0 }
            model::Office::Id id(json_office.at(CONST::ID).as_string().c_str());

            model::Point point{
                .x = static_cast<int>(json_office.at(CONST::X).as_int64()),
                .y = static_cast<int>(json_office.at(CONST::Y).as_int64()),
            };

            model::Offset offset{
                .dx = static_cast<int>(
                    json_office.at(CONST::OFFSET_X).as_int64()),
                .dy = static_cast<int>(
                    json_office.at(CONST::OFFSET_Y).as_int64()),
            };

            model::Office office(id, point, offset);

            map.AddOffice(std::move(office));
        }
    }
}

model::Game LoadGame(const std::filesystem::path &json_map_path) {
    std::ifstream file(json_map_path, std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " +
                                 json_map_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_str = buffer.str();

    model::Game game;

    boost::json::value json_value = boost::json::parse(json_str);
    for (auto &json_map : json_value.at(CONST::MAPS).as_array()) {
        model::Map map = GetBasicMapData(json_map);

        FillRoads(map, json_map);
        FillBuildings(map, json_map);
        FillOffices(map, json_map);

        game.AddMap(std::move(map));
    }

    return game;
}

std::string GetAllMapsInfoAsJsonString(const model::Game &game) {
    boost::json::array array;

    for (const auto &map : game.GetMaps()) {
        boost::json::object obj;
        obj[CONST::ID] = *map.GetId();
        obj[CONST::NAME] = map.GetName();
        array.emplace_back(std::move(obj));
    }

    return boost::json::serialize(array);
}

std::string GetMapInfoAsJsonString(const model::Map &map) {
    boost::json::object json_map;

    json_map[CONST::ID] = *map.GetId();
    json_map[CONST::NAME] = map.GetName();

    {
        boost::json::array roads;
        for (const auto &road : map.GetRoads()) {
            boost::json::object json_road;

            {
                auto [x, y] = road.GetStart();
                json_road[CONST::X0] = x;
                json_road[CONST::Y0] = y;
            }

            auto [x, y] = road.GetEnd();

            if (road.IsHorizontal()) {
                json_road[CONST::X1] = x;
            } else {
                json_road[CONST::Y1] = y;
            }

            roads.emplace_back(std::move(json_road));
        }

        json_map[CONST::ROADS] = std::move(roads);
    }

    {
        boost::json::array buildings;
        for (const auto &building : map.GetBuildings()) {
            boost::json::object json_building;

            auto [position, size] = building.GetBounds();

            json_building[CONST::X] = position.x;
            json_building[CONST::Y] = position.y;
            json_building[CONST::W] = size.width;
            json_building[CONST::H] = size.height;

            buildings.emplace_back(std::move(json_building));
        }

        json_map[CONST::BUILDINGS] = std::move(buildings);
    }

    {
        boost::json::array offices;
        for (const auto &office : map.GetOffices()) {
            boost::json::object json_office;

            auto [x, y] = office.GetPosition();
            auto [dx, dy] = office.GetOffset();

            json_office[CONST::ID] = *office.GetId();
            json_office[CONST::X] = x;
            json_office[CONST::Y] = y;
            json_office[CONST::OFFSET_X] = dx;
            json_office[CONST::OFFSET_Y] = dy;

            offices.emplace_back(std::move(json_office));
        }

        json_map[CONST::OFFICES] = std::move(offices);
    }

    return boost::json::serialize(json_map);
}

} // namespace json_loader
