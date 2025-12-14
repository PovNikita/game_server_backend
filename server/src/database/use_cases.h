#pragma once

#include <vector>

#include "domain.h"
#include "unit_of_work.h"

#include <pqxx/pqxx>

namespace database{
namespace app{
class UseCases {
public:
    virtual void SavePlayer(const domain::Player& player) = 0;
    virtual std::vector<domain::Player> GetPlayersStat(uint64_t offset, uint64_t limit) = 0;

protected:
    ~UseCases() = default;

};

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(UnitOfWorkFactory& factory)
        : unit_of_work_factory_(factory)
    {}

    void SavePlayer(const domain::Player& player) override 
    {
        auto w = unit_of_work_factory_.CreateUnitOfWork();
        w->Player()->Save(player);
        w->Commit();
    }

    std::vector<domain::Player> GetPlayersStat(uint64_t offset, uint64_t limit) override 
    {
        auto w = unit_of_work_factory_.CreateUnitOfWork();
        auto players = w->Player()->GetPlayersStat(offset, limit);
        w->Commit();
        return players;
    }


private:

    UnitOfWorkFactory& unit_of_work_factory_;
};

}//namespace app
}//namespace database