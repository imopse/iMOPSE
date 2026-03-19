#include "CGPBNTGAFactory.h"
#include "../../../../../method/methods/MO/GPBNTGA/CGPBNTGA.h"

AMethod* CGPBNTGAFactory::CreateGPBNTGA(SConfigMap* cfg, AProblem& problem, AInitialization* init) {
    return new CGPBNTGA(problem, *init, cfg);
}