#include "cola.h"
#include "sir/analyze/dataflow/reaching.h"
#include "sir/analyze/dataflow/cfg.h"
#include "passes/timing.h"
#include "passes/unnest.h"
#include "passes/specialization.h"
#include "passes/symbols.h"
#include "passes/canon.h"

void registerCommonPasses(transform::PassManager *pm, string cfgFile) {
  pm->registerPass(make_unique<Timing>(cfgFile));
}

void registerBasicOptPasses(transform::PassManager *pm) {
  pm->registerPass(make_unique<SpecializeIterators>());
}

void unnest(transform::PassManager *pm) {
  pm->registerPass(make_unique<Unnest>());
  pm->registerPass(make_unique<CheckUnnested>());
}

/*pair<string,string> cfgRd(transform::PassManager *pm) {
  auto cfgKey = pm->registerAnalysis(make_unique<analyze::dataflow::CFAnalysis>());
  auto rdKey = pm->registerAnalysis(make_unique<analyze::dataflow::RDAnalysis>(cfgKey),
				    "", {cfgKey});
  return {cfgKey, rdKey};
}

void lcfg(transform::PassManager *pm) {
  auto keys = cfgRd(pm);
  pm->registerPass(make_unique<ConstructLCFG>(), "", {keys.first, keys.second});
}*/

void CoLa::addIRPasses(transform::PassManager *pm, bool debug) {
  if (cfgFile != "") 
    cerr << "Using config file: " << cfgFile << endl;
  registerCommonPasses(pm, cfgFile);
  if (debug) {
    pm->registerPass(make_unique<SetSymbols>());
    return;  
  }
  registerBasicOptPasses(pm);
  unnest(pm);
  pm->registerPass(make_unique<CanonBlocks>());
  pm->registerPass(make_unique<CanonViews>());
  pm->registerPass(make_unique<SetSymbols>());
}

