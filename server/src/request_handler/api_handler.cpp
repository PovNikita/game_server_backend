#include "api_handler.h"
#include "../json_handler/json_loader.h"
#include "../application/players.h"
#include "../logging/logger.h"
#include "../game/game_items.h"
#include "../database/domain.h"

#include <utility>
#include <chrono>
#include <deque>
#include <vector>
#include <optional>

namespace http_handler{

std::string GetAllMapsInJSON(const model::Game& game);
std::optional<std::string> GetMapConfigurationJSON(const model::Game& game, const std::string& map_id);


StringResponse ApiHandler::HandleApiRequest(StringRequest&& req) {
    StringResponse response;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    if(req.target().find(Endpoints::join_endpoint) != std::string_view::npos)
    {
        response = HandlePostJoinEndpoint(std::move(req));
    }
    else if(req.target().find(Endpoints::get_players_in_session) != std::string_view::npos)
    {
        response = HandleGetPlayersInSession(std::move(req));
    }
    else if(req.target().find(Endpoints::get_state) != std::string_view::npos)
    {
        response = HandleGetState(std::move(req));
    }
    else if(req.target().find(Endpoints::action) != std::string_view::npos)
    {
        response = HandleAction(std::move(req));
    }
    else if(req.target().find(Endpoints::tick) != std::string_view::npos)
    {
        response = HandleTickAction(std::move(req));
    }
    else if(req.target().find(Endpoints::maps_endpoint) != std::string_view::npos)
    {
        response = HandleGetMapsEndpoint(std::move(req));
    }
    else if(req.target().find(Endpoints::records) != std::string_view::npos)
    {
        response = HandleGetRecords(std::move(req));
    }
    else if(req.method() == http::verb::head)
    {
        response = MakeStringResponse(http::status::ok, ""s, req.version(), req.keep_alive(), ContentType::TEXT_HTML, "no-cache"sv);
    }
    else
    {
        response = FormErrorJsonResponse(req, http::status::bad_request, "badRequest"sv, "Bad request"sv, "no-cache"sv);
    }
    std::string content_type = std::string{response[http::field::content_type]};
    int response_code = response.result_int();
    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::chrono::nanoseconds dur = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    LogResponse({content_type, response_code}, dur);
    return response;
}

StringResponse ApiHandler::HandleGetMapsEndpoint(StringRequest&& req) {
    if(req.method() != http::verb::get && req.method() != http::verb::head)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only GET method is expected"sv, "no-cache"sv, "GET, HEAD"sv);
    }
    std::string map_id = std::string{req.target().substr(Endpoints::maps_endpoint.size())};
    if(map_id.size() == 0 || (map_id.size() == 1 && map_id.back() == '/'))
    {
        std::string html_response = GetAllMapsInJSON(game_);
        return MakeStringResponse(http::status::ok, html_response, req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);
    }
    map_id.erase(0, 1);
    if(map_id.back() == '/')
    {
        map_id.erase(map_id.size() - 1);
    }
    auto map_config = GetMapConfigurationJSON(game_, map_id);
    if(map_config.has_value())
    {
        return MakeStringResponse(http::status::ok, map_config.value(), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);
    }
    return FormErrorJsonResponse(req, http::status::not_found, "mapNotFound"sv, "Map not found"sv, "no-cache"sv);
}

bool IsThereMap(const model::Game& game, const std::string& map_id)
{
    auto map = game.FindMap(model::Map::Id{map_id});
    if(!map)
    {
        return false;
    }
    return true;
}

StringResponse ApiHandler::HandlePostJoinEndpoint(StringRequest&& req) {
    if(req.method() != http::verb::post)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only POST method is expected"sv, "no-cache"sv, "POST"sv);
    }
    namespace sys = boost::system;
    sys::error_code ec;
    boost::json::value json_body = boost::json::parse(req.body(), ec);
    if (ec || !json_body.as_object().contains("userName") || !json_body.as_object().contains("mapId")) {
        //ошибка парсинга
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Join game request parse error"sv, "no-cache"sv);    
    }
    if(json_body.as_object()["userName"].as_string().size() == 0)
    {
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Join game request parse error"sv, "no-cache"sv);
    }
    if(!IsThereMap(game_, json_body.as_object()["mapId"].as_string().c_str()))
    {
        return FormErrorJsonResponse(req, http::status::not_found, "mapNotFound"sv, "Map not found"sv, "no-cache"sv);
    }
    auto [token, player_id] = application_.JoinGame(model::Map::Id{json_body.as_object()["mapId"].as_string().c_str()}, json_body.as_object()["userName"].as_string().c_str());
    boost::json::object answer_obj {
        {"authToken", *token},
        {"playerId", player_id}
    };
    return MakeStringResponse(http::status::ok, boost::json::serialize(answer_obj), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);
}


std::optional<std::string> ParseAuthorization(const StringRequest& req) {
    auto it = req.find(http::field::authorization);
    if (it == req.end()) {
        return std::nullopt;
    }

    std::string_view value = it->value();

    auto pos = value.find(' ');
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    std::string scheme(value.substr(0, pos));
    if(scheme != "Bearer")
    {
        return std::nullopt;
    }
    std::string token(value.substr(pos + 1));

    if (scheme.empty() || token.empty()) {
        return std::nullopt;
    }

    return token;
}

std::optional<StringResponse> ApiHandler::CheckAuthorizationAndFormErrorResponse(const StringRequest& req, std::optional<std::string> token)
{
    if(!token.has_value() || (token.has_value() && token.value().size() != 32))
    {
        return FormErrorJsonResponse(req, http::status::unauthorized, "invalidToken"sv, "Authorization header is missing"sv, "no-cache"sv);
    }
    if(application_.FindPlayerByToken(app::Token(token.value())) == nullptr)
    {
        return FormErrorJsonResponse(req, http::status::unauthorized, "unknownToken"sv, "Player token has not been found"sv, "no-cache"sv);
    }
    return std::nullopt;
}

std::optional<StringResponse> ApiHandler::CheckAuthorizationAndContentJSONType(const StringRequest& req, std::optional<std::string> token)
{
    auto it = req.find(http::field::content_type);
    if (it == req.end() || it->value() != ContentType::APPLICATION_JSON) {
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Invalid content type"sv, "no-cache"sv);
    }
    if(auto resp = CheckAuthorizationAndFormErrorResponse(req, token); resp.has_value())
    {
        return resp.value();
    }
    return std::nullopt;
}

StringResponse ApiHandler::HandleGetPlayersInSession(StringRequest&& req) {
    if(req.method() != http::verb::get && req.method() != http::verb::head)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only GET or HEAD methods is expected"sv, "no-cache"sv, "GET, HEAD"sv);
    }
    auto token = ParseAuthorization(req);
    if(auto resp = CheckAuthorizationAndFormErrorResponse(req, token); resp.has_value())
    {
        return resp.value();
    }
    auto players_deque = application_.GetAllPlayersInSessionWithCurrentPlayer(app::Token(token.value()));
    boost::json::object players_json;
    for(auto player : players_deque)
    {
        players_json[std::to_string(player->GetDog()->GetId())] = boost::json::object{
            {"name", player->GetDog()->GetName()}
    };
    }
    return MakeStringResponse(http::status::ok, boost::json::serialize(players_json), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);

}

StringResponse ApiHandler::HandleGetState(StringRequest&& req) {
    if(req.method() != http::verb::get && req.method() != http::verb::head)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only GET or HEAD methods is expected"sv, "no-cache"sv, "GET, HEAD"sv);
    }
    auto token = ParseAuthorization(req);
    if(auto resp = CheckAuthorizationAndFormErrorResponse(req, token); resp.has_value())
    {
        return resp.value();
    }
    auto players_deque = application_.GetAllPlayersInSessionWithCurrentPlayer(app::Token(token.value()));
    boost::json::object players_json;
    for (auto player : players_deque)
    {
        auto pos = player->GetDog()->GetDogCoordinates();
        auto speed = player->GetDog()->GetDogSpeed();
        auto dir = player->GetDog()->GetDogDirectionString();
        boost::json::array loots_json_bag;
        auto loots = player->GetSession()->GetAllLoots();
        for(auto id : player->GetDog()->GetDogsBag().GetAllIds())
        {
            boost::json::object loot{
                {"id", id},
                {"type", loots.at(id)->GetType()}
            };
            loots_json_bag.push_back(loot);
        }

        players_json[std::to_string(player->GetDog()->GetId())] = boost::json::object{
            {"pos", { pos.x_, pos.y_}},
            {"speed", { speed.dx_, speed.dy_}},
            {"dir", dir},
            {"bag", loots_json_bag},
            {"score", player->GetDog()->GetScore()}
        };
    }
    auto session = application_.GetGameSessionByPlayer(app::Token(token.value()));
    auto loots = session->GetLoots();
    boost::json::object loots_json;
    for(int i = 0; i < loots.size(); ++i)
    {
        if(!session->IsBusyLoot(i))
        {
            const auto& loot = loots.at(i);
            boost::json::object loot_json;
            loot_json[json_loader::literals::loot_type_type] = loot->GetType();
            boost::json::array pos = {static_cast<double>(loot->GetPosition().x), static_cast<double>(loot->GetPosition().y)};
            loot_json[json_loader::literals::loot_pos] = pos;
            loots_json[std::to_string(i)] = loot_json;
        }
    }
    boost::json::object result_json{
        {"players",     players_json},
        {"lostObjects", loots_json}
    };
    return MakeStringResponse(http::status::ok, boost::json::serialize(result_json), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);
}

bool IsDirectionCorrect(const std::string& direction)
{
    return (direction == "U" || direction == "D" || direction == "L" || direction == "R" || direction == "");
}

StringResponse ApiHandler::HandleAction(StringRequest&& req) {
    if(req.method() != http::verb::post)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only Post method is expected"sv, "no-cache"sv, "POST"sv);
    }
    auto token = ParseAuthorization(req);
    if(auto resp = CheckAuthorizationAndContentJSONType(req, token); resp.has_value())
    {
        return resp.value();
    }
    namespace sys = boost::system;
    sys::error_code ec;
    boost::json::value json_body = boost::json::parse(req.body(), ec);
    if (ec || !json_body.as_object().contains("move")) {
        //ошибка парсинга
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Failed to parse action"sv, "no-cache"sv);    
    }
    else if(std::string direction = json_body.as_object().at("move").as_string().c_str(); !(IsDirectionCorrect(direction)))
    {
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Failed to parse action"sv, "no-cache"sv);    
    }
    auto player = application_.FindPlayerByToken(app::Token(token.value()));
    player->MoveAction(json_body.as_object().at("move").as_string().c_str(), player->GetSession()->GetMap()->GetDogSpeed());

    return MakeStringResponse(http::status::ok, "{}", req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);

}

StringResponse ApiHandler::HandleTickAction(StringRequest&& req) 
{
    if(req.method() != http::verb::post)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only Post method is expected"sv, "no-cache"sv, "POST"sv);
    }
    auto it = req.find(http::field::content_type);
    if (it == req.end() || it->value() != ContentType::APPLICATION_JSON) {
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Invalid content type"sv, "no-cache"sv);
    }
    namespace sys = boost::system;
    sys::error_code ec;
    boost::json::value json_body = boost::json::parse(req.body(), ec);
    if (ec || !json_body.as_object().contains("timeDelta")) {
        //ошибка парсинга
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Failed to parse tick request JSON"sv, "no-cache"sv);    
    }
    else if(const boost::json::value time_delta_val = json_body.as_object().at("timeDelta"); !time_delta_val.is_int64() || time_delta_val.as_int64() < 0)
    {
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Failed to parse tick request JSON"sv, "no-cache"sv);    
    }
    if(application_.IsTickerRunning())
    {
        return FormErrorJsonResponse(req, http::status::bad_request, "badRequest"sv, "Invalid endpoint"sv, "no-cache"sv);    
    }
    application_.UpdateApplication(static_cast<uint64_t>(json_body.as_object().at("timeDelta").as_int64()));

    return MakeStringResponse(http::status::ok, "{}", req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);
}

std::pair<uint64_t, uint64_t> GetOffsetMaxItemsFromUrl(std::string_view url_string) {
    uint64_t offset = 0;
    uint64_t limit = 0;

    size_t pos_ques = url_string.find('?');
    if (pos_ques == std::string_view::npos)
        return {offset, limit}; // нет параметров

    // --- start ---
    if (size_t pos = url_string.find("start=", pos_ques); pos != std::string_view::npos) {
        pos += 6; // пропускаем "start="
        size_t end = url_string.find('&', pos);
        std::string_view value = url_string.substr(pos, end - pos);
        try {
            offset = std::stoull(std::string(value));
        } catch (...) {
            offset = 0;
        }
    }

    // --- maxItems ---
    if (size_t pos = url_string.find("maxItems=", pos_ques); pos != std::string_view::npos) {
        pos += 9; // пропускаем "maxItems="
        size_t end = url_string.find('&', pos);
        std::string_view value = url_string.substr(pos, end - pos);
        try {
            limit = std::stoull(std::string(value));
        } catch (...) {
            limit = 0;
        }
    }
    return {offset, limit};
}

StringResponse ApiHandler::HandleGetRecords(StringRequest&& req)
{
    static const double ms_to_sec = 0.001;
    if(req.method() != http::verb::get)
    {
        return FormErrorJsonResponse(req, http::status::method_not_allowed, "invalidMethod"sv, "Only Get method is expected"sv, "no-cache"sv, "GET"sv);
    }
    auto [offset, limit] = GetOffsetMaxItemsFromUrl(req.target());
    if(limit > application_.max_limit_records)
    {
        return FormErrorJsonResponse(req, http::status::bad_request, "invalidArgument"sv, "Limit can't be more than 100"sv, "no-cache"sv);   
    }
    std::vector<database::domain::Player> players_stat = application_.GetPlayersStats(offset, (limit == 0) ? application_.max_limit_records : limit);
    
    boost::json::array stat_json;
    for(const auto& player : players_stat){
        boost::json::object json_obj;
        json_obj.emplace("name", player.GetName());
        json_obj.emplace("score", player.GetScore());
        json_obj.emplace("playTime", static_cast<double>(player.GetTotalActiveTime()) * ms_to_sec);
        stat_json.push_back(json_obj);
    }

    return MakeStringResponse(http::status::ok, boost::json::serialize(stat_json), req.version(), req.keep_alive(), ContentType::APPLICATION_JSON, "no-cache"sv);
}

std::string GetAllMapsInJSON(const model::Game& game)
{
    using namespace std::literals;
    boost::json::array map_lists;
    for(auto map : game.GetMaps())
    {
        boost::json::object map_obj;
        map_obj[json_loader::literals::id] = *map.GetId();
        map_obj[json_loader::literals::map_name] = map.GetName();
        map_lists.push_back(map_obj);
    }
    return std::move(boost::json::serialize(map_lists));
}
void AddRoadsToMapJsonObject(boost::json::object& map_obj, const model::Map& map)
{
    boost::json::array roads;
    for(auto road : map.GetRoads())
    {
        boost::json::object road_obj;
        road_obj[json_loader::literals::x0_cor] = road.GetStart().x;
        road_obj[json_loader::literals::y0_cor] = road.GetStart().y;
        if(road.IsHorizontal())
        {
            road_obj[json_loader::literals::x1_cor] = road.GetEnd().x;
        }
        else
        {
            road_obj[json_loader::literals::y1_cor] = road.GetEnd().y;
        }
        roads.push_back(road_obj);
    }
    map_obj["roads"] = roads;
}

void AddBuildingsToMapJsonObject(boost::json::object& map_obj, const model::Map& map)
{
    boost::json::array buildings;
    for(auto building : map.GetBuildings())
    {
        boost::json::object building_obj;
        building_obj[json_loader::literals::x_cor] = building.GetBounds().position.x;
        building_obj[json_loader::literals::y_cor] = building.GetBounds().position.y;
        building_obj[json_loader::literals::width] = building.GetBounds().size.width;
        building_obj[json_loader::literals::height] = building.GetBounds().size.height;
        buildings.push_back(building_obj);
    }
    map_obj["buildings"] = buildings;
}

void AddOfficesToMapJsonObject(boost::json::object& map_obj, const model::Map& map)
{
    boost::json::array offices;
    for(auto office : map.GetOffices())
    {
        boost::json::object office_obj;
        office_obj[json_loader::literals::id] = *office.GetId();
        office_obj[json_loader::literals::x_cor] = office.GetPosition().x;
        office_obj[json_loader::literals::y_cor] = office.GetPosition().y;
        office_obj[json_loader::literals::x_offset] = office.GetOffset().dx;
        office_obj[json_loader::literals::y_offset] = office.GetOffset().dy;
        offices.push_back(office_obj);
    }
    map_obj["offices"] = offices;
}

void AddLootTypesToMapJsonObject(boost::json::object& map_obj, const model::Map& map)
{
    boost::json::array lootTypes;
    for(const auto& loot_type : map.GetLootTypes())
    {
        lootTypes.push_back(loot_type.GetLootType().as_object());
    }
    map_obj["lootTypes"] = lootTypes;
}

boost::json::object TransformMapConfigurationToJsonObkect(const model::Map& map)
{
    boost::json::object map_obj;
    map_obj[json_loader::literals::id] = *map.GetId();
    map_obj[json_loader::literals::map_name] = map.GetName();
    AddRoadsToMapJsonObject(map_obj, map);
    AddBuildingsToMapJsonObject(map_obj, map);
    AddOfficesToMapJsonObject(map_obj, map);
    AddLootTypesToMapJsonObject(map_obj, map);
    return map_obj;
}

std::optional<std::string> GetMapConfigurationJSON(const model::Game& game, const std::string& map_id)
{
    auto map = game.FindMap(model::Map::Id{map_id});
    if(!map)
    {
        return std::nullopt;
    }
    return boost::json::serialize(TransformMapConfigurationToJsonObkect(*map));
}

}