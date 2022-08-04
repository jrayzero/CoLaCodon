#include "cola.h"
#include "sir/analyze/dataflow/reaching.h"
#include "sir/analyze/dataflow/cfg.h"
#include "passes/timing.h"
#include "passes/unnest.h"
#include "passes/specialization.h"
#include "passes/symbols.h"
#include "passes/canon.h"
#include "passes/checks.h"
#include "passes/collapse.h"
#include "passes/utils.h"

void registerCommonPasses(transform::PassManager *pm, string cfgFile) {
  pm->registerPass(make_unique<CheckColaPrivate>());
  pm->registerPass(make_unique<Timing>(cfgFile));
}

void registerBasicOptPasses(transform::PassManager *pm) {
  pm->registerPass(make_unique<SpecializeIterators>());
}

pair<string,string> cfgRd(transform::PassManager *pm) {
  auto cfgKey = pm->registerAnalysis(make_unique<analyze::dataflow::CFAnalysis>());
  auto rdKey = pm->registerAnalysis(make_unique<analyze::dataflow::RDAnalysis>(cfgKey), {cfgKey});
  return {cfgKey, rdKey};
}

void removeTemporaries(transform::PassManager *pm, string cfgFile) {
  pm->registerPass(make_unique<UnnestCertainCoLaFuncs>());
  auto rdCfg = cfgRd(pm);
  string cfgKey = rdCfg.first;
  string rdKey = rdCfg.second;
  pm->registerPass(make_unique<RemoveExtraneousViews>(), "", {}, {cfgKey,rdKey});
  pm->registerPass(make_unique<PrintFuncs>(cfgFile));
  pm->registerPass(make_unique<CopyPropViews>(rdKey), "", {cfgKey,rdKey}, {cfgKey,rdKey});  
}


void CoLa::addIRPasses(transform::PassManager *pm, bool debug) {  
  if (cfgFile != "") 
    cerr << "Using config file: " << cfgFile << endl;  
  pm->registerPass(make_unique<PrintFuncs>(cfgFile));
  registerCommonPasses(pm, cfgFile);
  if (debug) {
    pm->registerPass(make_unique<SetSymbols>());
    return;  
  }
  registerBasicOptPasses(pm);
  removeTemporaries(pm, cfgFile);

//  pm->registerPass(make_unique<CanonBlocks>());
//  pm->registerPass(make_unique<CanonViews>());
  pm->registerStandardPasses(false);
  pm->registerPass(make_unique<SetSymbols>());
  pm->registerPass(make_unique<PrintFuncs>(cfgFile));
}

