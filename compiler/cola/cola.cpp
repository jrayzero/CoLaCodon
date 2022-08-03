#include "cola.h"
#include "passes/timing.h"
#include "passes/unnest.h"

void CoLa::registerCommonPasses(transform::PassManager *pm) {
  pm->registerPass(make_unique<Timing>(cfgFile));
}

void unnest(transform::PassManager *pm) {
  pm->registerPass(make_unique<Unnest>());
  pm->registerPass(make_unique<CheckUnnested>());
}

void CoLa::addIRPasses(transform::PassManager *pm, bool debug) {
  if (cfgFile != "") 
    cerr << "Using config file: " << cfgFile << endl;
  registerCommonPasses(pm);
  if (debug)
    return;  
  unnest(pm);
}

