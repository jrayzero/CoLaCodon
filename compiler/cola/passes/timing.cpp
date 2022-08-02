#include <iostream>
#include <fstream>
#include <algorithm>
#include "timing.h"
#include "utils.h"
#include "modules.h"
#include "clike.h"
#include "sir/util/irtools.h"
#include "sir/util/cloning.h"

const string Timing::KEY = "cola-timing-pass";

struct InsertTimingCalls : public util::Operator {

  InsertTimingCalls(VarValue *timerStart, Var *timerAccum, Func *parentFunc) : 
    timerStart(timerStart), timerAccum(timerAccum), parentFunc(parentFunc) { }

  void handle(ReturnInstr *instr) override {
    auto *module = instr->getModule();
    auto *series = module->Nr<SeriesFlow>();
    auto *timer = module->getOrRealizeFunc("timer", {}, {}, colaUtilsModule);
    auto *timerStop = util::makeVar(util::call(timer, {}), series, 
				    cast<BodiedFunc>(parentFunc));
    auto *diff = *timerStop - *timerStart;
    auto *accum = *diff + *module->Nr<VarValue>(timerAccum);
    auto *update = module->Nr<AssignInstr>(timerAccum, accum);
    series->push_back(update);
    auto *newRet = module->Nr<ReturnInstr>(instr->getValue());
    see(newRet);
    series->push_back(newRet);
    instr->replaceAll(series);
  }

private:

  VarValue *timerStart;
  Var *timerAccum;
  Func *parentFunc;
};

Timing::Timing(string cfgFile) {
  fstream cfgFd;
  cfgFd.open(cfgFile, ios::in);
  if (cfgFd.is_open()) {
    string line;
    while(getline(cfgFd, line)) {
      vector<string> tokens = splitString(line, "=");
      if (tokens.size() < 2) continue;
      string tok0 = toLower(trim(tokens[0]));
      string tok1 = trim(tokens[1]);
      if (tok0 == "time") {
	funcsToTime.insert(tok1);
      }
    }
    cfgFd.close();
  } else if (!cfgFile.empty()) {
    cerr << "Could not open config file: " << cfgFile << endl;
    exit(-1);
  }
}

BodiedFunc *makeEmptyFunc(Module *module, string name) {
  auto *funcType = module->getFuncType(module->getVoidType(), {});
  auto *func = module->Nr<BodiedFunc>(name);
  func->setUnmangledName(name);
  func->realize(funcType, {});
  return func;
}

void Timing::visit(Module *module) {
  for (auto t : funcsToTime) {
    LOG_IR("[{}] Looking to time {}.", KEY, t);
  }
  util::Operator::visit(module);
  if (alreadyTimed.empty()) return;
  // need to create a new function that initializes all of the globals
  auto *inits = module->Nr<SeriesFlow>();
  for (auto kv : alreadyTimed) {
    inits->push_back(module->Nr<AssignInstr>(kv.second.second, module->getInt(0)));
  }
  auto *initFunc = makeEmptyFunc(module, "__init_timing_globals");
  initFunc->setBody(inits);
  // create a function that print outs of the timing information
  auto *reports = module->Nr<SeriesFlow>();
  auto *printer = CHECK(module->getOrRealizeFunc("sprinter", {module->getStringType()}, 
						 {}, colaUtilsModule));
  reports->push_back(util::call(printer, {module->getString("=============")}));
  for (auto kv : alreadyTimed) {
    auto *reporter = CHECK(module->getOrRealizeFunc("report_time", 
						    {module->getStringType(),
						     module->getIntType()}, {}, 
						    colaUtilsModule));
    reports->push_back(util::call(reporter, {module->getString(kv.second.first),
					     module->Nr<VarValue>(kv.second.second)}));
  }
  auto *reportFunc = makeEmptyFunc(module, "__report_timing");
  reportFunc->setBody(reports);
  // call the new functions from the global module
  auto *mainBody = CHECK(cast<SeriesFlow>(CHECK(cast<BodiedFunc>(module->getMainFunc()))->getBody()));
  mainBody->insert(mainBody->begin(), util::call(initFunc, {}));
  mainBody->push_back(util::call(reportFunc, {}));
}

void Timing::handle(CallInstr *instr) {
  auto *func = cast<BodiedFunc>(util::getFunc(instr->getCallee()));  
  if (!func) return;
  string uname = func->getUnmangledName();
  string mname = func->getName();
  auto *module = instr->getModule();
  auto *body = module->Nr<SeriesFlow>();
  if (funcsToTime.count(uname) > 0 && alreadyTimed.count(mname) == 0) {
    if (isGeneratorType(func->getType())) {
      LOG_IR("[{}] Not timing {} because it is a generator.", KEY, uname);
      return;
    }
    // TODO need to initialize this to 0 in a function within main for something
    auto *timerAccum = getGlobalVar(module, module->getIntType());
    alreadyTimed[mname] = {uname,timerAccum};
    auto *timer = CHECK(module->getOrRealizeFunc("timer", {}, {}, colaUtilsModule));
    auto *timerStart = util::makeVar(util::call(timer, {}), body, func);
    body->push_back(func->getBody());
    // go through the function body and insert a timer stop + timer incr wherever
    // control returns from the function
    InsertTimingCalls inserter(timerStart, timerAccum, func);
    body->accept(inserter);
    func->setBody(body);
    LOG_IR("[{}] Inserted timing for function {}.", KEY, uname);
  }
}
