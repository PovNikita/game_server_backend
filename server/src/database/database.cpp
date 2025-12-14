#include "database.h"

namespace database
{
    Database::Database(const char* db_url, unsigned num_threads)
    : db_url_(db_url)
    , num_threads_(num_threads)
    , connection_pool_(num_threads, [db_url] {return std::make_shared<pqxx::connection>(db_url);})
    , unit_of_work_factory_(connection_pool_)
    {
        std::unique_ptr<ConnectionPool::ConnectionWrapper> conn = std::make_unique<ConnectionPool::ConnectionWrapper>(connection_pool_.GetConnection());
        pqxx::work w(*(*conn.get()));
        w.exec(
                "CREATE TABLE IF NOT EXISTS retired_players (id UUID PRIMARY KEY, name varchar(100) NOT NULL, score integer NOT NULL, play_time_ms integer NOT NULL);"_zv
        );

        w.exec(
                "CREATE INDEX IF NOT EXISTS sort_recorder_index ON retired_players (score DESC, play_time_ms, name);"_zv
        );

        w.commit();
    }

    ConnectionPool& Database::GetConnectionPool() 
    {
        return connection_pool_;
    }

    const char* Database::GetDbUrl() const 
    {
        return db_url_;
    }

    unsigned Database::GetNumThreads() const
    {
        return num_threads_;
    }

    database::postgres::UnitOfWorkFactoryImpl& Database::GetUnitOfWorkImpl()
    {
        return unit_of_work_factory_;
    }
} // namespace database