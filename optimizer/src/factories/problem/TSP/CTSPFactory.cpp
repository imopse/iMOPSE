#include "CTSPFactory.h"

const std::string CTSPFactory::s_Delimiter = " ";
const std::string CTSPFactory::s_DimensionKey = "DIMENSION:";
const std::string CTSPFactory::s_CitiesSectionKey = "NODE_COORD_SECTION";

CTSPTemplate *CTSPFactory::tspTemplate = nullptr;