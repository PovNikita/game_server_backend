#define _USE_MATH_DEFINES

#include "../src/game/collision_detector.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>
#include <string>
#include <limits>


const std::string TAG = "[FindGatherEvents]";

namespace collision_detector_tests{

using namespace collision_detector;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

static const double EPSILON = 1e-10;

class ItemGathererProviderTest : public ItemGathererProvider
{
public:
    using Gatherers = std::vector<Gatherer>;
    using Items = std::vector<Item>;

    ItemGathererProviderTest(Items items, Gatherers gatherers)
    : gatherers_(std::move(gatherers))
    , items_(std::move(items))
    {}

    size_t ItemsCount() const override
    {
        return items_.size();
    }

    Item GetItem(size_t idx) const override
    {
        return items_.at(idx);
    }

    size_t GatherersCount() const override
    {
        return gatherers_.size();
    }

    Gatherer GetGatherer(size_t idx) const override
    {
        return gatherers_.at(idx);
    }

private:
    Gatherers gatherers_;
    Items items_;
};


TEST_CASE("FindGatherEventsWithoutItemsAnd/OrGatherers", TAG)
{
    Gatherer gatherer = {.start_pos = {0.0, 0.0}, .end_pos = {5.0, 5.0}, .width = 0.6};
    Item item = {.position = {0.0, 0.0}, .width = 0.6};

    SECTION("Without items and gatherers")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({}, {}));
        CHECK(collision_list.size() == 0);
    }

    SECTION("Without items")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({}, {gatherer}));
        CHECK(collision_list.size() == 0);
    }

    SECTION("Without gatherers")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item}, {}));
        CHECK(collision_list.size() == 0);
    }
}

TEST_CASE("Item(-s) is on the way of a gatherer", TAG)
{
    Gatherer gatherer = {.start_pos = {0.0, 0.0}, .end_pos = {5.0, 5.0}, .width = 0.6};
    Item item1 = {.position = {0.0, 0.0}, .width = 0.6};
    Item item2 = {.position = {2.5, 2.5}, .width = 0.6};
    Item item3 = {.position = {5.0, 5.0}, .width = 0.6};

    SECTION("Item is at the middle of the way")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item2}, {gatherer}));
        CHECK(collision_list.size() == 1);
    }

    SECTION("Item is at the beginning of the way")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item1}, {gatherer}));
        CHECK(collision_list.size() == 1);
    }

    SECTION("Item is at the end of the way")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item3}, {gatherer}));
        CHECK(collision_list.size() == 1);
    }

    SECTION("Items are on the way")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item1, item2, item3}, {gatherer}));
        REQUIRE(collision_list.size() == 3);
        CHECK(collision_list.at(0).item_id == 0);
        CHECK(collision_list.at(1).item_id == 1);
        CHECK(collision_list.at(2).item_id == 2);
        CHECK(collision_list.at(0).gatherer_id == 0);
        CHECK(collision_list.at(1).gatherer_id == 0);
        CHECK(collision_list.at(2).gatherer_id == 0);
    }
}

TEST_CASE("Items are not on the way", TAG)
{
    Gatherer gatherer = {.start_pos = {0.0, 0.0}, .end_pos = {5.0, 5.0}, .width = 0.6};
    Item item1 = {.position = {-2.0, -2.0}, .width = 0.6};
    Item item2 = {.position = {2.5, 2.5}, .width = 0.6};

    SECTION("Item isn't on the way")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item1}, {gatherer}));
        CHECK(collision_list.size() == 0);
    }

    SECTION("At least one item isn't on the way")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item1, item2}, {gatherer}));
        REQUIRE(collision_list.size() == 1);
        CHECK(collision_list.at(0).item_id == 1);
    }
}

TEST_CASE("Gatherer doesn't move", TAG)
{
    Gatherer gatherer = {.start_pos = {5.0, 5.0}, .end_pos = {5.0, 5.0}, .width = 0.6};
    Item item1 = {.position = {5.0, 5.0}, .width = 0.6};
    auto collision_list = FindGatherEvents(ItemGathererProviderTest({item1}, {gatherer}));
    CHECK(collision_list.size() == 0);
}

TEST_CASE("Distance between item and gatherer is less then 2xWidth")
{
    const double width = 0.6;
    const double collision_distance = 2 * width - std::numeric_limits<double>::epsilon();
    Item item = {.position = {2.5, 2.5}, .width = width};

    SECTION("Horizontal") {
        Gatherer gatherer {
            .start_pos = {item.position.x + collision_distance, 0.0},
            .end_pos = {item.position.x + collision_distance, item.position.y},
            .width = width
        };

        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item}, {gatherer}));

        REQUIRE(collision_list.size() == 1);
        CHECK(collision_list.at(0).item_id == 0);
        CHECK(collision_list.at(0).gatherer_id == 0);
        CHECK_THAT(collision_list.at(0).sq_distance, WithinRel(collision_distance * collision_distance, EPSILON));
        CHECK_THAT(collision_list.at(0).time,
            WithinRel(item.position.y / gatherer.end_pos.y, EPSILON)
        );
    }

    SECTION("Vertical") {
        Gatherer gatherer {
            .start_pos = {0.0, item.position.y + collision_distance},
            .end_pos = {item.position.x, item.position.y + collision_distance},
            .width = width
        };

        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item}, {gatherer}));

        REQUIRE(collision_list.size() == 1);

        CHECK(collision_list.at(0).item_id == 0);
        CHECK(collision_list.at(0).gatherer_id == 0);
        CHECK_THAT(collision_list.at(0).sq_distance, WithinRel(collision_distance * collision_distance, EPSILON));
        CHECK_THAT(
            collision_list.at(0).time,
            WithinRel(item.position.x / gatherer.end_pos.x, EPSILON)
        );
    }
}

TEST_CASE("Collision distance is 2xWidth", TAG) {
    const double width = 0.6;
    const double collision_distance = width * 2;

    Item item {.position = {2.5, 2.5}, .width = width};

    SECTION("Horizontal") {
        Gatherer gatherer {
            .start_pos = {item.position.x + collision_distance, 0.0},
            .end_pos = {item.position.x + collision_distance, item.position.y},
            .width = width,
        };

        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item}, {gatherer}));

        CHECK(collision_list.empty());
    }

    SECTION("Vertical") {
        Gatherer gatherer {
            .start_pos = {0.0, item.position.y + collision_distance},
            .end_pos = {item.position.x, item.position.y + collision_distance},
            .width = width
        };

        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item}, {gatherer}));

        CHECK(collision_list.empty());
    }
}

TEST_CASE("Several gatherers", TAG) {
    Gatherer gatherer1 = {.start_pos = {0.0, 0.0}, .end_pos = {5.0, 5.0}, .width = 0.6};
    Gatherer gatherer2 = {.start_pos = {2.5, 2.5}, .end_pos = {-1.0, -1.0}, .width = 0.6};
    Gatherer gatherer3 = {.start_pos = {0.0, 0.0}, .end_pos = {-5.0, 5.0}, .width = 0.6};
    Item item = {.position = {2.5, 2.5}, .width = 0.6};

    SECTION("Two gatherers should cacth item and one shouldn't")
    {
        auto collision_list = FindGatherEvents(ItemGathererProviderTest({item}, {gatherer1, gatherer2, gatherer3}));

        REQUIRE(collision_list.size() == 2);
        CHECK(collision_list.at(0).item_id == 0);
        CHECK(collision_list.at(0).gatherer_id == 1);
        CHECK_THAT(collision_list.at(0).sq_distance, WithinAbs(0.0, EPSILON));
        CHECK_THAT(collision_list.at(0).time, WithinRel( (item.position.x - gatherer2.start_pos.x) / (gatherer2.end_pos.x - gatherer2.start_pos.x), EPSILON));

        CHECK(collision_list.at(1).item_id == 0);
        CHECK(collision_list.at(1).gatherer_id == 0);
        CHECK_THAT(collision_list.at(1).sq_distance, WithinAbs(0.0, EPSILON));
        CHECK_THAT(collision_list.at(1).time, WithinRel(item.position.x / gatherer1.end_pos.x, EPSILON));
    }
}

}// end of namespace collision_detector_tests

// Напишите здесь тесты для функции collision_detector::FindGatherEvents