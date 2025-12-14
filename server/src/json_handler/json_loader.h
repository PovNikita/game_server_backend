#pragma once

#include <filesystem>
#include <optional>

#include "../game/game.h"

namespace json_loader {

namespace literals {
    static const char *id                   = "id";
    static const char *x_cor                = "x";
    static const char *x0_cor               = "x0";
    static const char *x1_cor               = "x1";
    static const char *x_offset             = "offsetX";
    static const char *y_cor                = "y";
    static const char *y0_cor               = "y0";
    static const char *y1_cor               = "y1";
    static const char *y_offset             = "offsetY";
    static const char *width                = "w";
    static const char *height               = "h";
    static const char *map_name             = "name";
    static const char *loot_type_name       = "name";
    static const char *loot_type_file_path  = "file";
    static const char *loot_type_type       = "type";
    static const char *loot_type_rotation   = "rotation";
    static const char *loot_type_color      = "color";
    static const char *loot_type_scale      = "scale";
    static const char *loot_pos             = "pos"; 
}


model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
