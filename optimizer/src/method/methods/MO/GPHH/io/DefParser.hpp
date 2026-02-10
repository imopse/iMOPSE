#pragma once
#include <string>
#include "../domain/Instance.hpp"

class DefParser {
public:
    static bool parseFile(const std::string& path, Instance& out);
};
