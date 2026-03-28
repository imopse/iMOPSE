#include "CBNTGAGPFactory.h"
#include "../../../../../method/methods/MO/BNTGAGP/CBNTGAGP.h"

AMethod* CBNTGAGPFactory::CreateBNTGAGP(SConfigMap* cfg, AProblem& problem, AInitialization* init) {
    return new CBNTGAGP(problem, *init, cfg);
}