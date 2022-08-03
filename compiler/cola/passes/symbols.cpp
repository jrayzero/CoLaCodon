#include "symbols.h"

const string SetSymbols::KEY = "cola-set-symbols-pass";

void SetSymbols::processVar(Var *var) {
  if (!var) return;
  if (var->isGlobal()) return;
  auto *parent = cast<BodiedFunc>(getParentFunc());
  if (!parent) return;
  // only add it once
  for (auto *sym : *parent) {
    if (sym->getId() == var->getId()) return;
  }
  parent->push_back(var);  
}

void SetSymbols::handle(VarValue *vv) {
  processVar(vv->getVar());
}

void SetSymbols::handle(PointerValue *vv) {
  processVar(vv->getVar());
}

void SetSymbols::handle(AssignInstr *instr) {
  processVar(instr->getLhs());
}

void SetSymbols::handle(ForFlow *flow) {
  processVar(flow->getVar());
}

void SetSymbols::handle(ImperativeForFlow *flow) {
  processVar(flow->getVar());
}

void SetSymbols::handle(TryCatchFlow *flow) {
  for (auto &ctch : *flow) {
    auto *var = ctch.getVar();
    processVar(var);
  }
}

void SetSymbols::visit(BodiedFunc *func) {
  if (seen.count(func->getBody()->getId()) > 0) return;
  seen.insert(func->getBody()->getId());
  if (func->begin() != func->end())
    func->erase(func->begin());
  func->getBody()->accept(*this);
}
