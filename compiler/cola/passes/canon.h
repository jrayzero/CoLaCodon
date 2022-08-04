#pragma once
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

struct CanonBlocks : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(CallInstr *instr) override;
};

struct CanonViews : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(CallInstr *instr) override;
};

/*// Look for calls to Block.__new__/View.__new__ followed
// by Block.__init__/View.__init__ and combine them into one.
// This should be run early on since it depends on the flowinstr output
// from the front-end.
struct CombineNewInit : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(FlowInstr *instr) override;
};
*/
