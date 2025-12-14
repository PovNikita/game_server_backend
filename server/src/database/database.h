#pragma once

//pqxx
#include <pqxx/pqxx>
#include <pqxx/connection>

#include "./postgres.h"
#include "./connection_pool.h"


namespace database {

using pqxx::operator"" _zv;
using namespace std::literals;
static const char* DB_ENV_NAME = "GAME_DB_URL";

class Database {
public:
    Database() = delete;
    explicit Database(const char* db_url, unsigned num_threads);

    ConnectionPool& GetConnectionPool();

    const char* GetDbUrl() const;

    unsigned GetNumThreads() const;

    database::postgres::UnitOfWorkFactoryImpl& GetUnitOfWorkImpl();

private:
    ConnectionPool connection_pool_;
    const char* db_url_;
    const unsigned num_threads_;
    database::postgres::UnitOfWorkFactoryImpl unit_of_work_factory_;
};

} //namespace database