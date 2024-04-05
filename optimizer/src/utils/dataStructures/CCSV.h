#pragma once

#include <vector>


#include <ostream>
#include <fstream>
#include <algorithm>
#include <sstream>

template <typename T>
class CCSV
{
public:
    CCSV(size_t columnsCount)
        : m_ColumnsCount(columnsCount)
    {}

    // TODO - add column count validation
    void AddRow(const std::vector<T>& newRow) { m_Data.push_back(newRow); }
    void AddRow(std::vector<T>&& newRow) { m_Data.push_back(newRow); }

    std::ostringstream ToStringStream()
    {
        std::ostringstream ostringstream;
        ToCSV(ostringstream, m_Data);
        return ostringstream;
    }

    static void ToCSV(std::ostringstream& ostringstream, const std::vector<std::vector<T>>& csvData)
    {
        for (const auto& row: csvData)
        {
            for (size_t i = 0; i < row.size(); ++i)
            {
                ostringstream << row[i];
                if (i < row.size() - 1)
                {
                    ostringstream << ";";
                }
            }
            ostringstream << "\n";
        }
    }

private:
    size_t m_ColumnsCount;
    std::vector<std::vector<T>> m_Data;
};