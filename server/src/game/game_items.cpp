#include "game_items.h"
#include <stdexcept>

namespace model {

bool Road::IsPointPartOfRoad(double x, double y) const noexcept {
    auto right_lower_point = CalculateRightLowerPoint();
    auto left_top_point = CalculateLeftTopPoint();
    if((static_cast<double>(left_top_point.x) <= x) && (x <= static_cast<double>(right_lower_point.x)))
    {
        if((static_cast<double>(left_top_point.y) <= y) && (static_cast<double>(right_lower_point.y) >= y))
        {
            return true;
        }
    }
    return false;
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}
}