#pragma once

#include "dsl/dsl.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

struct CoLa : public DSL {
public:
  string getName() const override { return "CoLa"; }
  void addIRPasses(ir::transform::PassManager *pm, bool debug) override;
};