#pragma once
#include <vector>
#include <deque>
#include <string>

namespace Logical
{
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
        std::size_t maxRows = 10000;
        
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

}