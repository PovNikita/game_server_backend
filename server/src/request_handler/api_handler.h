#pragma once
#include <boost/json.hpp>

#include <string>

#include "request_utils.h"
#include "../game/game.h"
#include "../application/application.h"

namespace http_handler{
class ApiHandler{
public:
    explicit ApiHandler(model::Game& game, app::Application& application) : game_{game}, application_(application) {};

    ApiHandler(const ApiHandler&) = delete;
    ApiHandler& operator=(const ApiHandler&) = delete;

    ApiHandler(ApiHandler&& other) = default;
    ApiHandler& operator=(ApiHandler&& other) = default;

    static bool IsApiRequest(const StringRequest& request)
    {
        if(request.target().find(Endpoints::api_endpoint) != std::string_view::npos)
        {
            return true;
        }
        return false;
    }
    StringResponse HandleApiRequest(StringRequest&& request);
private:
    model::Game& game_;
    app::Application& application_;
    StringResponse HandleGetMapsEndpoint(StringRequest&& request);
    StringResponse HandlePostJoinEndpoint(StringRequest&& request);
    StringResponse HandleGetPlayersInSession(StringRequest&& request);
    StringResponse HandleGetState(StringRequest&& request);
    StringResponse HandleAction(StringRequest&& request);
    StringResponse HandleTickAction(StringRequest&& request);
    StringResponse HandleGetRecords(StringRequest&& request);

    std::optional<StringResponse> CheckAuthorizationAndFormErrorResponse(const StringRequest& req, std::optional<std::string> token);
    std::optional<StringResponse> CheckAuthorizationAndContentJSONType(const StringRequest& req, std::optional<std::string> token);
};
}