#include "cola.h"
#include "passes/timing.h"

void CoLa::registerCommonPasses(transform::PassManager *pm) {
  pm->registerPass(make_unique<Timing>(cfgFile));
}

void CoLa::addIRPasses(transform::PassManager *pm, bool debug) {
  if (cfgFile != "") 
    cerr << "Using config file: " << cfgFile << endl;
  registerCommonPasses(pm);
  if (debug)
    return;
}

