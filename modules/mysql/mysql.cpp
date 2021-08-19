#include <Logical.hpp>
#include <mysql/mysql.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>

using namespace Logical;

// TODO: Have a connection pool, potentially.

class MySqlTable
{
public:
    std::string name;
    int numberOfColumns;
    MySqlTable(const std::string & name, Int cols) : name(name), numberOfColumns(cols) {}
};

static void addrow(Call & call);
static void querytable(Call & call);

class MySqlConnection
{
public:
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
    
    void sync(Module & module)
    {
        auto q = query("SHOW TABLES");
        
        MYSQL_RES *result = mysql_use_result(&mysql);

        if(!result) return;
        
        int count = mysql_field_count(&mysql);
        
        while(auto row = mysql_fetch_row(result))
        {
            auto lengths = mysql_fetch_lengths(result);
            tables.push_back(row[0]);
        }
        
        mysql_free_result(result);
        
        for(auto & table : tables)
        {
            // !! SQL injection
            q = query(("DESCRIBE " + table).c_str());
            result = mysql_use_result(&mysql);
            int count = mysql_field_count(&mysql);
            auto lengths = mysql_fetch_lengths(result);
            
            std::vector<std::string> names;
            std::string prefix = "mysql:" + table + ":";
            
            while(auto row = mysql_fetch_row(result))
            {
                auto name = prefix + row[0];
                auto type = row[1];
                names.push_back(name);
                std::cout << "Row " << name << " has type " << type << std::endl;
                // Now create a predicate
    
            }
            
            mysql_free_result(result);

            const char * nameArray[names.size()];
            std::vector<Mode> mode(names.size());
            for(int i=0; i<names.size(); ++i)
            {
                nameArray[i] = names[i].c_str();
                mode[i] = Out;
            }
            
            
            auto data = new MySqlTable(table, names.size());
            //module.AddCommand(addrow, nameArray, data);
            
            //module.AddFunction(querytable, nameArray, mode, data);
        }
    }
} connection;

void Write(std::ostream & os, Call &c, int i)
{
    const char * strval;
    Int ival;
    double dval;
    
    if(c.Get(i, strval))
    {
        int len = std::strlen(strval);
        char escaped[len+1];
        mysql_escape_string(escaped, strval, len);
        os << "\"" << escaped << "\"";
    }
    else if(c.Get(i, ival))
    {
        os << ival;
    }
    else if(c.Get(i, dval))
    {
        os << dval;
    }
    else
    {
        os << "NULL";
    }
}

static void addrow(Call & call)
{
    auto data = (MySqlTable*)call.GetData();
    std::stringstream ss;
    ss << "INSERT INTO " << data->name << " VALUES (";
    for(int i=0; i<data->numberOfColumns; ++i)
    {
        if(i>0) ss << ", ";
        Write(ss, call, i);
    }
    
    ss << ")";
    
    if(connection.query(ss.str().c_str()))
        call.GetModule().ReportError(connection.error());
}

static void querytable(Call & call)
{
    auto data = (MySqlTable*)call.GetData();
    std::stringstream ss;
    ss << "SELECT * FROM " << data->name;
    connection.query(ss.str().c_str());
    
    auto result = mysql_use_result(&connection.mysql);
    int count = mysql_field_count(&connection.mysql);

    while(auto row = mysql_fetch_row(result))
    {
        for(int i=0; i<count; ++i)
            if(row[i])
                call.Set(i, row[i]);
            else
                call.Set(i, "");
        call.YieldResult();
    }
    
    mysql_free_result(result);

}

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

static void sync(Call & call)
{
    connection.sync(call.GetModule());
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
    
    module.AddCommand(connectdb, {"mysql:database", "user"});
    
    module.AddCommand(mysql_exec, {"mysql:execute"});
    module.AddCommand(sync, {"mysql:sync"});
    module.AddCommand(sync, {"mysql"});
    module.AddFunction(error, {"mysql:error"}, {Out});
    
    module.AddFunction(table1, {"mysql:table"}, {Out});
}
