#pragma once
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

// Set function symbols for functions all at once
struct SetSymbols : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void handle(VarValue *vv) override;
  void handle(PointerValue *vv) override;
  void handle(AssignInstr *instr) override;
  void handle(ForFlow *flow) override;
  void handle(ImperativeForFlow *flow) override;
  void handle(TryCatchFlow *flow) override;
  void visit(BodiedFunc *func) override;
  
 private:
  set<ir::id_t> seen;
  void processVar(Var *var);
};
