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
