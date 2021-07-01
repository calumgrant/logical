#include <Logical.hpp>
#include <mysql/mysql.h>
#include <iostream>

using namespace Logical;

// TODO: Have a connection pool, potentially.
MYSQL mysql;

static void connectdb(Call & call)
{
    const char * dbname, *username;
    if(call.Get(1, username) && call.Get(0, dbname))
    {
        auto r = mysql_real_connect(&mysql, nullptr, username, nullptr, dbname, 0, nullptr, 0);
        if(!r)
        {
            call.GetModule().ReportError(mysql_error(&mysql));
        }
    }
}

static void mysql_exec(Call & call)
{
    const char * sql;
    if(call.Get(0, sql))
    {
        if(mysql_query(&mysql, sql))
        {
            call.GetModule().ReportError(mysql_error(&mysql));
        }
    }
}

static void error(Call & call)
{
    auto error = mysql_error(&mysql);
    if(error && *error)
    {
        call.Set(0, error);
        call.YieldResult();
    }
}

void RegisterFunctions(Module & module)
{
    mysql_init(&mysql);
    
    module.AddCommand(connectdb, "database", "mysql:user");
    
    module.AddCommand(mysql_exec, "mysql:execute");
    module.AddFunction(error, "mysql:error", Out);
}
