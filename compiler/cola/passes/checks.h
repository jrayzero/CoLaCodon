#pragma once
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

struct CheckColaPrivate : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(CallInstr *instr) override;
};
