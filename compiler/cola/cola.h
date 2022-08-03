#pragma once

#include "dsl/dsl.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

struct CoLa : public DSL {
public:
  explicit CoLa(string cfgFile) : cfgFile(cfgFile) { }
  string getName() const override { return "CoLa"; }
  void addIRPasses(transform::PassManager *pm, bool debug) override;
private:
  string cfgFile;
};
