#include "cola.h"

void CoLa::addIRPasses(transform::PassManager *pm, bool debug) {
  if (cfgFile != "") 
    cerr << "Using config file: " << cfgFile << endl;
  if (debug)
    return;
}

