#pragma once
#include "sir/sir.h"
#include "sir/types/types.h"

using namespace std;
using namespace seq::ir;

class CLike : public util::ConstVisitor {
public:
  CLike() : indent_amt(0), did_indent(false) { }
  string get() const { return last; }
  template <typename T>
  string format(const T *obj) {
    obj->accept(*this);
    return get();
  }
  void defaultVisit(const Node *) override { }

protected:
  void visit(const Module *) override;
  void visit(const BodiedFunc *) override;
  void visit(const ExternalFunc *) override;
  void visit(const InternalFunc *) override;
  void visit(const LLVMFunc *) override;
  void visit(const Var *) override;
  void visit(const VarValue *) override;
  void visit(const PointerValue *) override;

  void visit(const IntConst *) override;
  void visit(const FloatConst *) override;
  void visit(const BoolConst *) override;
  void visit(const StringConst *) override;

  void visit(const SeriesFlow *) override;
  void visit(const IfFlow *) override;
  void visit(const WhileFlow *) override;
  void visit(const ForFlow *) override;
  void visit(const ImperativeForFlow *) override;
  void visit(const TryCatchFlow *) override;

  void visit(const AssignInstr *) override;
  void visit(const ExtractInstr *) override;
  void visit(const InsertInstr *) override;
  void visit(const CallInstr *) override;
  void visit(const StackAllocInstr *) override;
  void visit(const TypePropertyInstr *) override;
  void visit(const YieldInInstr *) override;
  void visit(const TernaryInstr *) override;
  void visit(const BreakInstr *) override;
  void visit(const ContinueInstr *) override;
  void visit(const ReturnInstr *) override;
  void visit(const YieldInstr *) override;
  void visit(const ThrowInstr *) override;
  void visit(const FlowInstr *) override;

  void visit(const types::IntType *) override;
  void visit(const types::FloatType *) override;
  void visit(const types::BoolType *) override;
  void visit(const types::ByteType *) override;
  void visit(const types::VoidType *) override;
  void visit(const types::RecordType *) override;
  void visit(const types::RefType *) override;
  void visit(const types::FuncType *) override;
  void visit(const types::OptionalType *) override;
  void visit(const types::PointerType *) override;
  void visit(const types::GeneratorType *) override;
  void visit(const types::IntNType *) override;

  int indent_amt;
  bool did_indent;
  void incr_ind();
  void decr_ind();
  string do_ind() const;
  string last;
  bool ends_in_nl = false;
  vector<types::Type*> visited_func_types;
  vector<string> visited_func_names;
  vector<const Var*> vars;
};

