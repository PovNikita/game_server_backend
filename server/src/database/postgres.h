#pragma once

#include <optional>
#include <memory>

#include "./domain.h"
#include "./unit_of_work.h"
#include "./connection_pool.h"

#include <pqxx/pqxx>

namespace database {

namespace postgres
{

using pqxx::operator"" _zv;

class PlayerRepositoryImpl : public domain::PlayerRepository {
public:
    PlayerRepositoryImpl(pqxx::work& w)
    : w_(w) 
    {}

    void Save(const domain::Player& player) override;
    void Delete(std::string_view name) override;
    std::optional<domain::Player> GetPlayerByName(std::string_view name) override;
    std::vector<domain::Player> GetPlayersStat(uint64_t offset, uint64_t limit) override;


private:
    pqxx::work& w_;

};

class UnitOfWorkImpl : public app::UnitOfWork {
public:
    explicit UnitOfWorkImpl(ConnectionPool::ConnectionWrapper&& connection) 
    : connection_(std::move(connection))
    {}

    void Commit() override 
    {
        w_.commit();
    }

    domain::PlayerRepository* Player() override 
    {
        return &player_repos_;
    }

    ~UnitOfWorkImpl() = default;

  private:
    ConnectionPool::ConnectionWrapper connection_;
    pqxx::work w_{*connection_};
    PlayerRepositoryImpl player_repos_{w_};
};

class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(ConnectionPool& connection_pool)
    : connection_pool_(connection_pool) 
    {}

    std::unique_ptr<app::UnitOfWork> CreateUnitOfWork() override 
    {
        return std::make_unique<UnitOfWorkImpl>(connection_pool_.GetConnection());
    }

private:
    ConnectionPool& connection_pool_;
};

} // namespace postgres



}// namespace database