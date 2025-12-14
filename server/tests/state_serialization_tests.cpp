#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "../src/game/game_items.h"
#include "../src/serialization/serialization.h"

using namespace model;
using namespace std::literals;
namespace {

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

}  // namespace

SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const model::DoublePoint p{10, 20};
        WHEN("point is serialized") {
            output_archive << p;

            THEN("it is equal to point after serialization") {
                InputArchive input_archive{strm};
                model::DoublePoint restored_point;
                input_archive >> restored_point;
                CHECK(p == restored_point);
            }
        }
    }
}

SCENARIO_METHOD(Fixture, "Dog Serialization") {
    GIVEN("a dog") {
        const auto dog = [] {
            Dog dog("Pluto"s, 3);
            dog.SetId(42);
            dog.SetDogCoordinates({42.2, 12.5});
            dog.SetScore(42);
            dog.GetDogsBag().AddIdOfTheLoot(1);
            dog.SetDogDirection(Direction::EAST);
            dog.SetDogSpeed({2.3, -1.2});
            return dog;
        }();

        WHEN("dog is serialized") {
            {
                serialization::DogRepr repr(dog, "token", "map_id");
                output_archive << repr;
            }

            THEN("it can be deserialized") {
                InputArchive input_archive{strm};
                serialization::DogRepr repr;
                input_archive >> repr;
                const auto [restored_dog, token, map_id] = repr.Restore();

                CHECK(dog.GetId() == restored_dog.GetId());
                CHECK(dog.GetName() == restored_dog.GetName());
                CHECK(dog.GetDogCoordinates() == restored_dog.GetDogCoordinates());
                CHECK(dog.GetDogSpeed() == restored_dog.GetDogSpeed());
                CHECK(dog.GetBagCapacity() == restored_dog.GetBagCapacity());
                CHECK(dog.GetDogsBag().GetNumberOfItems() == restored_dog.GetDogsBag().GetNumberOfItems());
            }
        }
    }
}
