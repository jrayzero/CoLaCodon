#pragma once

#include <stack>
#include "sir/sir.h"
#include "sir/transform/pass.h"

using namespace std;
using namespace seq;
using namespace seq::ir;

// Unnest all arguments so everything is in a three-address code form
// While loops also modified so the condition is moved within the loop since
// it has to run each time.

struct Unnest : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
  void visit(AssignInstr *instr) override;
  void visit(ExtractInstr *instr) override;
  void visit(InsertInstr *instr) override;
  void visit(ReturnInstr *instr) override;
  void visit(YieldInstr *instr) override;
  void visit(ThrowInstr *instr) override;
  void visit(CallInstr *instr) override;
  void visit(FlowInstr *instr) override;
  void visit(SeriesFlow *flow) override;
  void visit(WhileFlow *flow) override;
  void visit(ForFlow *flow) override;
  void visit(ImperativeForFlow *flow) override;
  void visit(IfFlow *flow) override;
  void visit(TryCatchFlow *flow) override;
  void visit(PipelineFlow *flow) override;
protected:
  stack<SeriesFlow*> ctx;
};

struct CheckUnnested : public transform::OperatorPass {
  static const string KEY;
  string getKey() const override { return KEY; }
//  void handle(AssignInstr *instr) override;
//  void handle(ExtractInstr *instr) override;
//  void handle(InsertInstr *instr) override;
//  void handle(ReturnInstr *instr) override;
//  void handle(YieldInstr *instr) override;
//  void handle(ThrowInstr *instr) override;
  void handle(CallInstr *instr) override;
  void handle(FlowInstr *instr) override;
  void handle(WhileFlow *flow) override;
  void handle(ForFlow *flow) override;
  void handle(ImperativeForFlow *flow) override;
  void handle(IfFlow *flow) override;
};
