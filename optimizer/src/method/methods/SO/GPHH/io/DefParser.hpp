#pragma once
#include <string>
#include "../domain/Instance.hpp"

namespace gphh_so {

class DefParser {
public:
    static bool parseFile(const std::string& path, Instance& out);
};

}                     