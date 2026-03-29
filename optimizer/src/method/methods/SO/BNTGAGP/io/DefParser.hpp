#pragma once
#include <string>
#include "../domain/Instance.hpp"

namespace gpbntga_so {

class DefParser {
public:
    static bool parseFile(const std::string& path, Instance& out);
};

} // namespace gpbntga_so