// A test of the externs API

#include "Logical.hpp"
#include <cassert>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <deque>

using namespace Logical;

static void helloworld(Call & call)
{
    call.Set(0, "Hello, world!");
    call.YieldResult();
}

static void countargs(Call & call)
{
    call.Set(0, (Int)call.ArgCount());
    call.YieldResult();
}

static void listargs(Call & call)
{
    int args = call.ArgCount();
    for(int i=0; i<args; ++i)
    {
        if(call.GetMode(i) == Out)
        {
            call.Set(i, call.ArgName(i));
        }
        else
        {
            const char * arg;
            call.Get(i, arg);
            if(strcmp(arg, call.ArgName(i))!=0)
                return;
        }
    }
    call.YieldResult();
}

static void concat(Call & call)
{
    std::ostringstream ss;
    int args = call.ArgCount();
    for(int i=1; i<args; ++i)
    {
        if(i>1) ss << " ";
        ss << call.ArgName(i) << "=";
        
        if(call.GetMode(i) == In)
        {
            const char * v;
            if(call.Get(i, v))
                ss << v;
            else ss << "?";
        }
        else
            ss << "_";
    }
    call.Set(0, ss.str().c_str());
    call.YieldResult();
}

static void setdata(Call & call)
{
    Call & getdata = call.GetModule().GetPredicate({"test:getdata"});

    Int data;
    if(call.Get(0, data))
    {
        getdata.Set(0, data);
        getdata.YieldResult();
    }
}

// A standalone class to write tables to a stream

struct TableWriterConfig
{
    // Line separator
    const char * newline = "\n";
    
    // Text at the beginning of each row
    const char * rowPrefix = "┃";
    
    // Text to separate columns
    const char * columnSeparator = "│";
    
    // Text at the end of each row
    const char * rowSuffix = "┃";

    // Text to put in front of each value
    const char * paddingPrefix = " ";
    
    // Text to put after each value, also used to align columns
    const char * paddingSuffix = " ";
    
    // The character to use in the top border, or "" to disable
    const char * topBorder = "━";
    const char * topBorderPrefix = "┏";
    const char * topBorderSuffix = "┓";
    const char * topBorderSeparator = "┯";
    
    const char * headerSeparator = "━";
    const char * headerSeparatorPrefix = "┣";
    const char * headerSeparatorSuffix = "┫";
    const char * headerSeparatorSeparator = "┿";
    
    // The character to use in the bottom border, or null/"" to disable
    const char * bottomBorder = "━";
    const char * bottomBorderPrefix = "┗";
    const char * bottomBorderSuffix = "┛";
    const char * bottomBorderSeparator = "┷";

    // If rows are unequal in length, what is the padding value to use for missing values
    const char * padValue = "";
    
    // The maximum width of any column, or 0 for no maximum value
    std::size_t maxWidth = 0;
    
    // Truncate long strings instead of flowing them into the next line
    bool truncate = false;
    
    // A suffix to indicate that the value has been truncated
    const char * truncateSuffix = "...";
    
    // The number of rows to buffer.
    std::size_t bufferSize = 100;
    
    // The maximum number of rows to display, or 0 to disable.
    std::size_t maxRows = 0;
    
    // Sort the data by the given column
    // Set to -1 to not sort the data.
    int sortColumn = 0;
    
    // Deduplicate identical rows
    bool deduplicate = true;
    
    // When set to true, pretty-print each row to have a constant width
    bool alignRows = true;
};

class TableWriter
{
public:
    virtual ~TableWriter();

    typedef std::vector<std::string> Row;

    virtual void Header(Row &&) =0;
    virtual void AddRow(Row &&) =0;
    virtual void EndTable() =0;
};

std::shared_ptr<TableWriter> CreateTableWriter(std::ostream & os, const TableWriterConfig&);

class TableWriterImpl : public TableWriter
{
public:
    TableWriterImpl(std::ostream & output, const TableWriterConfig & config);

    void Header(Row &&) override;
    void AddRow(Row &&) override;
    void EndTable() override;

private:
    void PrintRow(Row&);
    void ComputeWidths(const Row&);
    void PrintSeparator(const char * str, const char * prefix, const char * sep, const char * suffix);

    TableWriterConfig config;
    std::ostream & output;
    
    Row header;
    std::deque<Row> rowBuffer;
    std::vector<int> widths;
};

TableWriterImpl::TableWriterImpl(std::ostream & output, const TableWriterConfig & config) :
    output(output), config(config)
{
}

TableWriter::~TableWriter()
{
    
}

void TableWriterImpl::Header(Row &&row)
{
    header = std::move(row);
}

void TableWriterImpl::AddRow(Row &&row)
{
    rowBuffer.push_back(std::move(row));
}

void TableWriterImpl::EndTable()
{
    // Compute the widths
    ComputeWidths(header);
    
    
    for(auto & i : rowBuffer)
        ComputeWidths(i);
    
    // Print it all out
    PrintSeparator(config.topBorder, config.topBorderPrefix, config.topBorderSeparator, config.topBorderSuffix);
    PrintRow(header);
    PrintSeparator(config.headerSeparator, config.headerSeparatorPrefix, config.headerSeparatorSeparator, config.headerSeparatorSuffix);

    for(auto & i : rowBuffer)
        PrintRow(i);

    PrintSeparator(config.bottomBorder, config.bottomBorderPrefix, config.bottomBorderSeparator, config.bottomBorderSuffix);

    header.clear();
    rowBuffer.clear();
    widths.clear();
}

void TableWriterImpl::ComputeWidths(const Row & row)
{
    if(widths.size() < row.size())
        widths.resize(row.size());
    
    for(int i=0; i<row.size(); ++i)
    {
        if(widths[i] < row[i].size())
            widths[i] = row[i].size();
    }
}

void TableWriterImpl::PrintSeparator(const char *str, const char * prefix, const char *sep, const char * suffix)
{
    if(!str || !sep || !prefix || !suffix) return;
    
    output << prefix;
    bool first = true;
    for(auto & i : widths)
    {
        if(first)
            first = false;
        else
            output << sep;
        for(int j=0; j<i + strlen(config.paddingPrefix) + strlen(config.paddingSuffix); ++j)
            output << str;
    }
    output << suffix << config.newline;
}

void TableWriterImpl::PrintRow(Row & row)
{
    if(row.size() < widths.size())
        row.resize(widths.size(), config.padValue);
    
    for(int i=0; i<widths.size(); ++i)
    {
        if(i==0)
        {
            output << (config.rowPrefix ? config.rowPrefix : "");
        }
        else
        {
            output << (config.columnSeparator ? config.columnSeparator : "");
        }
        output << config.paddingPrefix;
        output << row[i];
        for(int p=row[i].size(); p<=widths[i]; p+= strlen(config.paddingSuffix))
            output << config.paddingSuffix;
    }
    output << config.rowSuffix << config.newline;
}

static void outputTable(Call & call)
{
    int args = call.ArgCount();
    auto & writer = *(TableWriter*)(call.GetData());

    if(call.First())
    {
        std::vector<std::string> row;
        row.push_back("");

        for(int i=1; i<args; ++i)
        {
            row.push_back(call.ArgName(i));
        }
        
        writer.Header(std::move(row));
        return;
    }
    
    if(call.Last())
    {
        writer.EndTable();
        std::cout << std::endl;
        return;
    }
    
    std::vector<std::string> row;
    for(int i=0; i<args; ++i)
    {
        if(call.GetMode(i) == In)
        {
            const char * v;
            Int iv;
            if(call.Get(i, v))
                row.push_back(v);
            else if(call.Get(i, iv))
            {
                std::stringstream ss;
                ss << iv;
                row.push_back(ss.str());
            }
            else
                row.push_back("?");
        }
    }
    call.CountResult();
    writer.AddRow(std::move(row));
}

void RegisterFunctions(Module & module)
{
    module.AddFunction(helloworld, {"test:helloworld"}, {Out});
    module.AddFunction(countargs, {"test:countargs"}, {Out});
    module.AddFunction(listargs, {"test:listargs", "arg1"}, {Out, Out});
    module.AddFunction(listargs, {"test:listargs"}, {Varargs});
    module.AddFunction(concat, {"test:concat"}, {Varargs});
    module.AddCommand(setdata, {"test:setdata"});

    auto tablewriter = new TableWriterImpl(std::cout, TableWriterConfig());
    module.AddFunction(outputTable, {"test:table"}, {Varargs}, tablewriter);
}
