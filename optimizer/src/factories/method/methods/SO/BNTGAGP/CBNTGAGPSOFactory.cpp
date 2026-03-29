#include "CBNTGAGPSOFactory.h"
#include "../../../../../method/methods/SO/BNTGAGP/CBNTGAGPSO.h"

AMethod* CBNTGAGPSOFactory::CreateBNTGAGPSO(SConfigMap* cfg, AProblem& problem, AInitialization* init) {
    return new CBNTGAGPSO(problem, *init, cfg);
}