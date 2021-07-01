#include <Logical.hpp>
#include <mysql/mysql.h>
#include <iostream>
#include <vector>
#include <string>

using namespace Logical;

// TODO: Have a connection pool, potentially.

class MySqlConnection
{
    MYSQL mysql;
    
public:
    void init()
    {
        mysql_init(&mysql);
    }
    
    bool connect(const char * username, const char * dbname)
    {
        return mysql_real_connect(&mysql, nullptr, username, nullptr, dbname, 0, nullptr, 0);

    }
    
    const char * error()
    {
        return mysql_error(&mysql);
    }
    
    int query(const char * q)
    {
        return mysql_query(&mysql, q);
    }
    
    class MappedTable
    {
        
    };
    
    std::vector<std::string> tables;
    
    void sync()
    {
        auto q = query("SHOW TABLES");
        
        MYSQL_RES *result = mysql_use_result(&mysql);
        int count = mysql_field_count(&mysql);
        
        while(auto row = mysql_fetch_row(result))
        {
            auto lengths = mysql_fetch_lengths(result);
            tables.push_back(row[0]);
        }
        
        mysql_free_result(result);
    }
} connection;



static void connectdb(Call & call)
{
    const char * dbname, *username;
    if(call.Get(1, username) && call.Get(0, dbname))
    {
        auto r = connection.connect(username, dbname);
        if(!r)
        {
            call.GetModule().ReportError(connection.error());
        }
    }
}

static void mysql_exec(Call & call)
{
    const char * sql;
    if(call.Get(0, sql))
    {
        if(connection.query(sql))
        {
            call.GetModule().ReportError(connection.error());
        }
    }
}

static void error(Call & call)
{
    auto error = connection.error();
    
    if(error && *error)
    {
        call.Set(0, error);
        call.YieldResult();
    }
}

void syncTables(Module & module)
{
    
}


static void sync(Call & call)
{
    connection.sync();
}

static void table1(Call & call)
{
    for(auto & t : connection.tables)
    {
        call.Set(0, t.c_str());
        call.YieldResult();
    }
}

void RegisterFunctions(Module & module)
{
    connection.init();
    
    module.AddCommand(connectdb, "database", "mysql:user");
    
    module.AddCommand(mysql_exec, "mysql:execute");
    module.AddCommand(sync, "mysql:sync");
    module.AddCommand(sync, "mysql");
    module.AddFunction(error, "mysql:error", Out);
    
    module.AddFunction(table1, "mysql:table", Out);
}
