#include "CGPHHFactory.h"
#include "../../../../../method/methods/MO/GPHH/CGPHH.h"

AMethod* CGPHHFactory::CreateGPHH(SConfigMap* cfg, AProblem& problem, AInitialization* init) {
    return new CGPHH(problem, *init, cfg);
}
