#pragma once
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

// Insert timing calls based on functions specified in cfg file. Inserts within the function
// and reports cumulative timing numbers.
// In cfg file: time=<funcname>
struct Timing : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override {return KEY; }
  explicit Timing(string cfgFile);

  void handle(CallInstr *instr) override;

  void visit(Module *module) override;

 private:
  // these are unmangled names
  set<string> funcsToTime;
  // these are mangled names
  // name => unmangled,global
  map<string,pair<string,Var*>> alreadyTimed;
};
