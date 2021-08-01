#include <TableWriter.hpp>
#include <iostream>

Logical::TableWriterImpl::TableWriterImpl(std::ostream & output, const TableWriterConfig & config) :
    output(output), config(config)
{
}

Logical::TableWriter::~TableWriter()
{
}

void Logical::TableWriterImpl::Header(Row &&row)
{
    header = std::move(row);
}

void Logical::TableWriterImpl::AddRow(Row &&row)
{
    if(config.maxRows>0 && rowBuffer.size()>=config.maxRows)
    {
        if(rowBuffer.size()==config.maxRows)
        {
            for(auto & r : row)
                r = "...";
            rowBuffer.push_back(std::move(row));
        }
        return;
    }
    
    rowBuffer.push_back(std::move(row));
}

void Logical::TableWriterImpl::EndTable()
{
    // Compute the widths
    ComputeWidths(header);
    
    for(auto & i : rowBuffer)
        ComputeWidths(i);
    
    // Print it all out
    PrintSeparator(config.topBorder, config.topBorderPrefix, config.topBorderSeparator, config.topBorderSuffix);
    
    if(widths.size()!=1 || header[0] !="")
    {
        PrintRow(header);
        PrintSeparator(config.headerSeparator, config.headerSeparatorPrefix, config.headerSeparatorSeparator, config.headerSeparatorSuffix);
    }
    
    for(auto & i : rowBuffer)
        PrintRow(i);

    PrintSeparator(config.bottomBorder, config.bottomBorderPrefix, config.bottomBorderSeparator, config.bottomBorderSuffix);

    header.clear();
    rowBuffer.clear();
    widths.clear();
}

void Logical::TableWriterImpl::ComputeWidths(const Row & row)
{
    if(widths.size() < row.size())
        widths.resize(row.size());
    
    for(int i=0; i<row.size(); ++i)
    {
        if(widths[i] < row[i].size())
            widths[i] = row[i].size();
    }
}

void Logical::TableWriterImpl::PrintSeparator(const char *str, const char * prefix, const char *sep, const char * suffix)
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

void Logical::TableWriterImpl::PrintRow(Row & row)
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
