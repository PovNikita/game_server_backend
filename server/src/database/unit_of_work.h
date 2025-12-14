#pragma once

#include <memory>

#include "./domain.h"


namespace database
{

namespace app
{

    class UnitOfWork {
    public:
        virtual domain::PlayerRepository* Player() = 0;
        virtual void Commit() = 0;

        virtual ~UnitOfWork() = default;
    };

    class UnitOfWorkFactory{
    public:
        virtual std::unique_ptr<UnitOfWork> CreateUnitOfWork() = 0;

    protected:
        ~UnitOfWorkFactory() = default;
    };
    
} // namespace app

} // namespace database
