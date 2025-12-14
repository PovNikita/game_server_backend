#include "collision_detector.h"
#include <cassert>
#include <algorithm>

namespace collision_detector {

CollectionResult TryCollectPoint(model::DoublePoint a, model::DoublePoint b, model::DoublePoint c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider, bool is_auto_indexing) {
    std::vector<GatheringEvent> gathering_events;

    for(size_t i = 0; i < provider.GatherersCount(); ++i)
    {
        auto gatherer = provider.GetGatherer(i);
        if(gatherer.start_pos != gatherer.end_pos)
        {
            for(size_t j = 0; j < provider.ItemsCount(); ++j)
            {
                auto item = provider.GetItem(j);
                auto collect_res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
                if(collect_res.IsCollected(gatherer.width + item.width))
                {
                    if(is_auto_indexing == true)
                        gathering_events.emplace_back(GatheringEvent{.item_id = j, .gatherer_id = i, .sq_distance = collect_res.sq_distance, .time = collect_res.proj_ratio});
                    else
                        gathering_events.emplace_back(GatheringEvent{.item_id = item.id_, .gatherer_id = gatherer.id_, .sq_distance = collect_res.sq_distance, .time = collect_res.proj_ratio});
                }
            }
        }
    }

    std::sort(gathering_events.begin(), gathering_events.end(), [](const GatheringEvent& lhs, const GatheringEvent& rhs)
            {
                return lhs.time < rhs.time;
            });
    return gathering_events;
}



}  // namespace collision_detector