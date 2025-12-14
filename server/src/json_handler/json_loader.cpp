#include "json_loader.h"
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace sys = boost::system;

namespace json_loader {

void AddRoadToTheGameMap(const boost::json::value& road, model::Map& game_map)
{
    if(road.as_object().contains("x1"))
    {
        game_map.AddRoad(model::Road(model::Road::HORIZONTAL, model::Point{static_cast<int>(road.as_object().at(literals::x0_cor).as_int64()), 
                                                                static_cast<int>(road.as_object().at(literals::y0_cor).as_int64())},
                                                                static_cast<int>(road.as_object().at(literals::x1_cor).as_int64())));
    }
    else
    {
        game_map.AddRoad(model::Road(model::Road::VERTICAL, model::Point{static_cast<int>(road.as_object().at(literals::x0_cor).as_int64()), 
                                                static_cast<int>(road.as_object().at(literals::y0_cor).as_int64())},
                                                static_cast<int>(road.as_object().at(literals::y1_cor).as_int64())));
    }
}

void AddBuildingToTheGameMap(const boost::json::value& building, model::Map& game_map)
{
    model::Rectangle rect_building(model::Point(building.as_object().at(literals::x_cor).as_int64(), building.as_object().at(literals::y_cor).as_int64()),
                        model::Size(building.as_object().at(literals::width).as_int64(), building.as_object().at(literals::height).as_int64()));
    game_map.AddBuilding(model::Building(rect_building));
}

void AddOfficeToTheGameMap(const boost::json::value& office, model::Map& game_map)
{
    model::Office::Id id{office.as_object().at(literals::id).as_string().c_str()};
    model::Point point(office.as_object().at(literals::x_cor).as_int64(), office.as_object().at(literals::y_cor).as_int64());
    model::Offset offset(office.as_object().at(literals::x_offset).as_int64(), office.as_object().at(literals::y_offset).as_int64());
    game_map.AddOffice(model::Office(id, point, offset));
}

void AddLootTypeToTheGameMap(const boost::json::value& loot_type_json, model::Map& game_map)
{
    model::LootType loot_type(loot_type_json);
    game_map.AddLootType(std::move(loot_type));
}

void AddMapToTheGame(const boost::json::value& map, model::Game& game, double default_dog_speed, uint64_t default_bag_capacity)
{
    model::Map game_map(model::Map::Id{std::string(map.as_object().at(literals::id).as_string().c_str())}, 
                                        std::string(map.as_object().at(literals::map_name).as_string().c_str()));
    for(auto road : map.as_object().at("roads").as_array())
    {
        AddRoadToTheGameMap(road, game_map);
    }

    for(auto building : map.as_object().at("buildings").as_array())
    {
        AddBuildingToTheGameMap(building, game_map);
    }

    for(auto office : map.as_object().at("offices").as_array())
    {
        AddOfficeToTheGameMap(office, game_map);
    }

    if(auto array = map.as_object().at("lootTypes").as_array(); array.size() > 0)
    {
        for(auto loot_type : array)
        {
            AddLootTypeToTheGameMap(loot_type, game_map);
        }
    }
    else
    {
        //invalid json file
        throw std::runtime_error("Invalid Json File. Map has to have at least one loot type");
    }

    auto pointer = map.as_object().if_contains("dogSpeed");
    if(pointer != nullptr)
    {
        game_map.SetDogSpeed(pointer->as_double());
    }
    else
    {
        game_map.SetDogSpeed(default_dog_speed);
    }
    if(auto bag_capacity_ptr = map.as_object().if_contains("bagCapacity"); bag_capacity_ptr != nullptr)
    {
        game_map.SetBagCapacity(static_cast<uint64_t>(bag_capacity_ptr->as_int64()));
    }
    else
    {
        game_map.SetBagCapacity(default_bag_capacity);
    }

    game.AddMap(game_map);
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    using namespace std::literals;
    model::Game game;
    std::ifstream json_file(json_path);
    if(!json_file.is_open())
    {
        throw std::runtime_error("Can't open json file");
    }
    std::string json_string = ""s;
    std::string temp;
    while(getline(json_file, temp)) {
        json_string += temp;
    }
    boost::json::value parsed_json;
    try
    {
        parsed_json = boost::json::parse(json_string);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        throw;
    }
    
    auto array_of_maps = parsed_json.as_object().at("maps").as_array();
    double default_speed = parsed_json.as_object().at("defaultDogSpeed").as_double();
    auto pointer_bag = parsed_json.as_object().if_contains("defaultBagCapacity");
    auto pointer_retirement_time = parsed_json.as_object().if_contains("dogRetirementTime");
    uint64_t default_bag_capacity = model::default_bag_capacity;
    if(pointer_bag != nullptr)
    {
        default_bag_capacity = static_cast<uint64_t>(parsed_json.as_object().at("defaultBagCapacity").as_int64());
    }
    if(pointer_retirement_time != nullptr)
    {
        if(double time_s = (*pointer_retirement_time).as_double(); time_s > 0)
        {
            game.SetRetirementTime(time_s);
        }
    }
    for(auto map : array_of_maps)
    {
        AddMapToTheGame(map, game, default_speed, default_bag_capacity);
    }
    game.SetLootGeneratorConf(parsed_json.as_object().at("lootGeneratorConfig").as_object().at("period").as_double(), 
                                parsed_json.as_object().at("lootGeneratorConfig").as_object().at("probability").as_double());
    
    return game;
}

}// namespace json_loader
